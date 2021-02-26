// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbsys/sys.h>

#include <pybricks/util_mp/pb_obj_helper.h>

#include "lib/utils/interrupt_char.h"
#include "lib/utils/pyexec.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/persistentcode.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/stream.h"

// REVISIT: We could modify the linker script like upstream MicroPython so that
// we can specify the stack size per hub in the linker scripts. Currently, since
// the heap is statically allocated here, the stack size is whatever is left in
// RAM after .data and .bss sections. But it would probably be better to specify
// the stack size and let the heap be whatever is leftover.

// defined in linker script
extern uint32_t _estack;
extern uint32_t _ebss;

#if MICROPY_ENABLE_GC
static char heap[PYBRICKS_HEAP_KB * 1024];
#endif

// Implementation for MICROPY_EVENT_POLL_HOOK
void pb_stm32_poll(void) {
    while (pbio_do_one_event()) {
    }

    mp_handle_pending(true);

    // There is a possible race condition where an interupt occurs and sets the
    // Coniki poll_requested flag after all events have been processed. So we
    // have a critical section where we disable interupts and check see if there
    // are any last second events. If not, we can call __WFI(), which still wakes
    // up the CPU on interrupt even though interrupts are otherwise disabled.
    mp_uint_t state = disable_irq();
    if (!process_nevents()) {
        __WFI();
    }
    enable_irq(state);
}

// User .mpy file can be up to 1/2 of heap size. The code loader makes a new
// (slightly modified) copy, so we need at least this much free.
// TODO: need to verify that loaded code can never be bigger that .mpy file.
#define MPY_MAX_BYTES (PYBRICKS_HEAP_KB * 1024 / 2)

static pbio_error_t wait_for_button_release(void) {
    pbio_error_t err;
    pbio_button_flags_t btn = PBIO_BUTTON_CENTER;
    while (btn & PBIO_BUTTON_CENTER) {
        err = pbio_button_is_pressed(&btn);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        pb_stm32_poll();
    }
    return PBIO_SUCCESS;
}

// Wait for data from an IDE
static pbio_error_t get_message(uint8_t *buf, uint32_t rx_len, int32_t time_out) {
    // Maximum time between two bytes/chunks
    const int32_t time_interval = 500;

    // Acknowledge at the end of each message or each data chunk
    const uint32_t chunk_size = 100;

    pbio_error_t err;

    // Initialize
    uint8_t checksum = 0;
    uint32_t rx_count = 0;
    mp_uint_t time_start = mp_hal_ticks_ms();
    mp_uint_t time_now;
    pbio_button_flags_t btn;

    while (true) {

        // Check if button is pressed
        err = pbio_button_is_pressed(&btn);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        if (btn & PBIO_BUTTON_CENTER) {
            // If so, wait for release
            err = wait_for_button_release();
            if (err != PBIO_SUCCESS) {
                return err;
            }
            // Cancel waiting for message
            return PBIO_ERROR_CANCELED;
        }

        // Current time
        time_now = mp_hal_ticks_ms();

        // Try to get one byte
        if (mp_hal_stdio_poll(MP_STREAM_POLL_RD)) {
            buf[rx_count] = mp_hal_stdin_rx_chr();
            // On success, reset timeout
            time_start = time_now;

            // Update checksum
            checksum ^= buf[rx_count];

            // Increment rx counter
            rx_count++;

            // When done, acknowledge with the checksum
            if (rx_count == rx_len) {
                mp_hal_stdout_tx_strn((const char *)&checksum, 1);
                return PBIO_SUCCESS;
            }

            // Acknowledge after receiving a chunk.
            if (rx_count % chunk_size == 0) {
                mp_hal_stdout_tx_strn((const char *)&checksum, 1);
                // Reset the checksum
                checksum = 0;
            }
        }

        // Check if we have timed out
        if (rx_count == 0) {
            // Use given timeout for first byte
            if (time_out != -1 && time_now - time_start > time_out) {
                return PBIO_ERROR_TIMEDOUT;
            }
        } else if (time_now - time_start > time_interval) {
            // After the first byte, apply much shorter interval timeout
            return PBIO_ERROR_TIMEDOUT;
        }

        // Keep polling
        pb_stm32_poll();
    }
}

// Defined in linker script
extern uint32_t _pb_user_mpy_size;
extern uint8_t _pb_user_mpy_data;

// If user says they want to send an MPY file this big (19 MB),
// assume they want REPL. This lets users get REPL by pressing
// spacebar four times, so that no special tools are required.
static const uint32_t REPL_LEN = 0x20202020;

// Get user program via serial/bluetooth
static uint32_t get_user_program(uint8_t **buf, uint32_t *free_len) {
    pbio_error_t err;
    *buf = NULL;
    *free_len = 0;

    // flush any buffered bytes from stdin
    while (mp_hal_stdio_poll(MP_STREAM_POLL_RD)) {
        mp_hal_stdin_rx_chr();
    }

    // Get the program length
    uint32_t len;
    err = get_message((uint8_t *)&len, sizeof(len), -1);

    // If button was pressed, return code to run script in flash
    if (err == PBIO_ERROR_CANCELED) {
        *buf = &_pb_user_mpy_data;
        return _pb_user_mpy_size;
    }

    // Handle other errors
    if (err != PBIO_SUCCESS) {
        return 0;
    }

    // Four spaces triggers REPL
    if (len == REPL_LEN) {
        return REPL_LEN;
    }

    // Assert that the length is allowed
    if (len > MPY_MAX_BYTES) {
        return 0;
    }

    // Allocate buffer for MPY file with known length
    *buf = m_malloc(len);
    if (*buf == NULL) {
        return 0;
    }

    // Get the program
    err = get_message(*buf, len, 500);

    // Did not receive a whole program, so discard it
    if (err != PBIO_SUCCESS) {
        m_free(*buf);
        return 0;
    }

    *free_len = len;
    return len;
}

static void run_user_program(uint32_t len, uint8_t *buf, uint32_t free_len) {

    if (len == 0) {
        mp_print_str(&mp_plat_print, ">>>> ERROR\n");
        return;
    }

    // Send a message to say we will run a program
    mp_print_str(&mp_plat_print, "\n>>>> RUNNING\n");

    // FIXME: Make sure to not gobble up first characters of user output
    // in the web IDE. Instead for now, delineate subsequent program output.
    // This is useful regardless, and in this case it's okay if a few dashes
    // are gobbled up.
    mp_print_str(&mp_plat_print, "--------------\n");

    if (len == REPL_LEN) {
        #if MICROPY_ENABLE_COMPILER
        pyexec_friendly_repl();
        #else
        mp_print_str(&mp_plat_print, "REPL not supported!\n");
        #endif // MICROPY_ENABLE_COMPILER
        return;
    }

    // Allow script to be stopped with hub button
    mp_hal_set_interrupt_char(3);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_reader_t reader;
        mp_reader_new_mem(&reader, buf, len, free_len);
        mp_raw_code_t *raw_code = mp_raw_code_load(&reader);
        mp_obj_t module_fun = mp_make_function_from_raw_code(raw_code, MP_OBJ_NULL, MP_OBJ_NULL);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }

    // Reset interrupt
    mp_hal_set_interrupt_char(-1);
}

// callback for when stop button is pressed
static void user_program_stop_func(void) {
    // we can only raise an exception if the VM is running
    // mp_interrupt_char will be either -1 or 0 when VM is not running
    if (mp_interrupt_char > 0) {
        mp_keyboard_interrupt();
    }
}

static bool user_program_stdin_event_func(uint8_t c) {
    if (c == mp_interrupt_char) {
        mp_keyboard_interrupt();
        return true;
    }

    return false;
}

static const pbsys_user_program_callbacks_t user_program_callbacks = {
    .stop = user_program_stop_func,
    .stdin_event = user_program_stdin_event_func,
};

int main(int argc, char **argv) {
    pbio_init();

soft_reset:
    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_ebss - 1024);

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif

    wait_for_button_release();

    // Send a message to say hub is idle
    mp_print_str(&mp_plat_print, ">>>> IDLE\n");

    // Receive an mpy-cross compiled Python script
    uint8_t *program;
    uint32_t free_len;
    uint32_t len = get_user_program(&program, &free_len);

    // FIXME: The WEB IDE currently confuses last checksum byte(s) with the
    // status messaging sent when program begins. So for now, add a brief
    // wait so WEB IDE can easily distinguish.
    mp_hal_delay_ms(150);

    // Get system hardware ready
    pbsys_prepare_user_program(&user_program_callbacks);
    // make sure any pending events are handled before starting MicroPython
    while (pbio_do_one_event()) {
    }

    mp_init();

    // Execute the user script
    run_user_program(len, program, free_len);

    // Uninitialize MicroPython and the system hardware
    mp_deinit();
    pbsys_unprepare_user_program();

    goto soft_reset;

    return 0;
}

// defined in lib/utils/gchelper*.s
uintptr_t gc_helper_get_regs_and_sp(uintptr_t *regs);

void gc_collect(void) {
    // start the GC
    gc_collect_start();

    // get the registers and the sp
    uintptr_t regs[10];
    uintptr_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void **)sp, ((uint32_t)&_estack - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

void mp_reader_new_file(mp_reader_t *reader, const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    mp_raise_OSError(MP_ENOENT);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
    while (1) {
        ;
    }
}

void NORETURN __fatal_error(const char *msg) {
    while (1) {
        ;
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
