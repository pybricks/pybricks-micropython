// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/light.h>
#include <pbsys/sys.h>

#include "pbobj.h"

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/persistentcode.h"
#include "lib/utils/pyexec.h"
#include "lib/utils/interrupt_char.h"

#include "py/mphal.h"

// FIXME: Decide whether or not to pre-allocate
// memory for logging instead or just disable
// logging altogether on some constrained ports.
void *malloc(size_t n) {
    return m_malloc(n);
}
void free(void *p) {
    m_free(p);
}

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[PYBRICKS_HEAP_KB * 1024];
#endif

static pbio_error_t wait_for_button_release() {
    pbio_error_t err;
    pbio_button_flags_t btn = PBIO_BUTTON_CENTER;
    while (btn & PBIO_BUTTON_CENTER) {
        err = pbio_button_is_pressed(&btn);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        MICROPY_EVENT_POLL_HOOK
    }
    return PBIO_SUCCESS;
}

// Wait for data from an IDE
static pbio_error_t get_message(uint8_t *buf, uint32_t rx_len, bool clear, int32_t time_out) {

    // Optionally clear existing buffer
    if (clear) {
        uint8_t c;
        while (pbsys_stdin_get_char(&c) != PBIO_ERROR_AGAIN) {
            MICROPY_EVENT_POLL_HOOK
        }
    }

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
        err = pbsys_stdin_get_char(&buf[rx_count]);

        if (err == PBIO_SUCCESS) {
            // On success, reset timeout
            time_start = time_now;

            // Update checksum
            checksum ^= buf[rx_count];

            // Increment rx counter
            rx_count++;

            // When done, acknowledge with the checksum
            if (rx_count == rx_len) {
                return pbsys_stdout_put_char(checksum);
            }

            // Acknowledge after receiving a chunk.
            if (rx_count % chunk_size == 0) {
                err = pbsys_stdout_put_char(checksum);
                if (err != PBIO_SUCCESS) {
                    return err;
                }
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
        }
        else if (time_now - time_start > time_interval) {
            // After the first byte, apply much shorter interval timeout
            return PBIO_ERROR_TIMEDOUT;
        }
        // Keep polling
        MICROPY_EVENT_POLL_HOOK
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
static uint32_t get_user_program(uint8_t **buf, bool *free_reader) {
    pbio_error_t err;

    // Get the program length
    uint8_t len_buf[4];
    err = get_message(len_buf, 4, true, -1);

    // If button was pressed, return code to run script in flash
    if (err == PBIO_ERROR_CANCELED) {
        *buf = &_pb_user_mpy_data;
        return _pb_user_mpy_size;
    }

    // Handle other errors
    if (err != PBIO_SUCCESS) {
        *buf = NULL;
        return 0;
    }

    // Convert to uint32
    uint32_t len = ((uint32_t) len_buf[0]) << 24 |
                   ((uint32_t) len_buf[1]) << 16 |
                   ((uint32_t) len_buf[2]) << 8 |
                   ((uint32_t) len_buf[3]);

    // Four spaces triggers REPL
    if (len == REPL_LEN) {
        *buf = NULL;
        return REPL_LEN;
    }

    // Assert that the length is allowed
    if (len > MPY_MAX_BYTES) {
        return 0;
    }

    // Allocate buffer for MPY file with known length
    uint8_t *mpy = m_malloc(len);
    if (mpy == NULL) {
        return 0;
    }

    // Get the program
    err = get_message(mpy, len, false, 500);

    // Did not receive a whole program, so discard it
    if (err != PBIO_SUCCESS) {
        m_free(mpy);
        len = 0;
    }

    // Success, so return script, length, and free reader
    *buf = mpy;
    *free_reader = true;
    return len;
}

static void run_user_program(uint32_t len, uint8_t *buf, bool free_reader) {

    if (len == 0) {
        mp_print_str(&mp_plat_print, ">>>> ERROR\n");
        return;
    }

    if (len == REPL_LEN) {
        #if MICROPY_ENABLE_COMPILER
        pyexec_friendly_repl();
        #else
        mp_print_str(&mp_plat_print, "REPL not supported!\n");
        #endif // MICROPY_ENABLE_COMPILER
        return;
    }

    // Send a message to say we will run a program
    mp_print_str(&mp_plat_print, "\n>>>> RUNNING\n");

    mp_reader_t reader;
    uint32_t free_len = free_reader ? len : 0;
    mp_reader_new_mem(&reader, buf, len, free_len);

    // Convert buf to raw code and do m_free(buf) in the process
    mp_raw_code_t *raw_code = mp_raw_code_load(&reader);

    // Allow script to be stopped with hub button
    mp_hal_set_interrupt_char(3);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t module_fun = mp_make_function_from_raw_code(raw_code, MP_OBJ_NULL, MP_OBJ_NULL);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // nlr_jump(nlr.ret_val);
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
    .stop           = user_program_stop_func,
    .stdin_event    = user_program_stdin_event_func,
};

static void pb_imports() {
    // Import hubs module
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_hubs);

    // Import other modules if enabled
    #if PYBRICKS_PY_IODEVICES
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_iodevices);
    #endif
    #if PYBRICKS_PY_PUPDEVICES
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_pupdevices);
    #endif
    #if PYBRICKS_PY_PARAMETERS
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_parameters);
    #endif
    #if PYBRICKS_PY_TOOLS
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_tools);
    #endif
    #if PYBRICKS_PY_ROBOTICS
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_robotics);
    #endif
}

int main(int argc, char **argv) {
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    pbio_init();

soft_reset:

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif

    wait_for_button_release();

    // Send a message to say hub is idle
    mp_print_str(&mp_plat_print, ">>>> IDLE\n");

    // Receive an mpy-cross compiled Python script
    uint8_t *program;
    bool free_reader = false;
    uint32_t len = get_user_program(&program, &free_reader);

    // Get system hardware ready
    pbsys_prepare_user_program(&user_program_callbacks);

    // Initialize MicroPython and run default imports
    mp_init();
    pb_imports();

    // Execute the user script
    run_user_program(len, program, free_reader);

    // Uninitialize MicroPython and the system hardware
    mp_deinit();
    pbsys_unprepare_user_program();

    goto soft_reset;

    pbio_deinit();

    return 0;
}

// defined in linker script
extern uint32_t _estack;
// defined in lib/utils/gchelper_m0.s
uintptr_t gc_helper_get_regs_and_sp(uintptr_t *regs);

void gc_collect(void) {
    // start the GC
    gc_collect_start();

    // get the registers and the sp
    uintptr_t regs[10];
    uintptr_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void**)sp, ((uint32_t)&_estack - sp) / sizeof(uint32_t));

    // end the GC
    gc_collect_end();

    // for debug during development
    gc_dump_info();
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

void nlr_jump_fail(void *val) {
    while (1);
}

void NORETURN __fatal_error(const char *msg) {
    while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
