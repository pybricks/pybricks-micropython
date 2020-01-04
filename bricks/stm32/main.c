// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 David Lechner
// Copyright (c) 2019 Laurens Valk

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

typedef enum {
    WAITING_FOR_FIRST_RELEASE,
    WAITING_FOR_PRESS,
    WAITING_FOR_SECOND_RELEASE
} waiting_for_t;

bool timed_out(uint32_t time_start, uint32_t time_out) {
    // If time_out is zero, we say it never times out.
    if (time_out == 0) {
        return false;
    }
    // Return true if more than time_out ms have elapsed
    // since time_start, otherwise return false.
    return mp_hal_ticks_ms()- time_start > time_out;
}

// wait for button to be pressed/released before starting program
bool wait_for_button_press(uint32_t time_out) {
    pbio_button_flags_t btn;
    waiting_for_t wait_for = WAITING_FOR_FIRST_RELEASE;
    uint32_t time_start = mp_hal_ticks_ms();

    // wait for button rising edge, then falling edge
    while (!timed_out(time_start, time_out)) {
        pbio_button_is_pressed(&btn);
        if (btn & PBIO_BUTTON_CENTER) {
            // step 2:
            // once we are sure the button is released, we wait for it to be
            // pressed (rising edge).
            if (wait_for == WAITING_FOR_PRESS) {
                wait_for = WAITING_FOR_SECOND_RELEASE;
            }
        }
        else {
            // step 1:
            // we have to make sure the button is released before waiting for
            // it to be pressed, otherwise programs would be restarted as soon
            // as they are stopped because the button is already pressed.
            if (wait_for == WAITING_FOR_FIRST_RELEASE) {
                wait_for = WAITING_FOR_PRESS;
            }
            // step 3:
            // after the button has been pressed, we need to wait for it to be
            // released (falling edge), otherwise programs would stop as soon
            // as they were started because the button is already pressed.
            else if (wait_for == WAITING_FOR_SECOND_RELEASE) {
                return !timed_out(time_start, time_out);
            }
        }
        while (pbio_do_one_event()) { }
        __WFI();
    }
    return false;
}

// Wait for data from an IDE
pbio_error_t get_message(uint8_t *buf, uint32_t rx_len, bool clear, int32_t time_out) {
    
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
    int32_t time_start = mp_hal_ticks_ms();
    int32_t time_now = time_start;

    while (true) {
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

#ifdef PYBRICKS_MPY_MAIN_MODULE
extern uint32_t __user_flash_start;

// Get user program stored in rom
static uint32_t get_user_program(uint8_t **buf) {

    wait_for_button_press(0);
    mp_print_str(&mp_plat_print, "\nLoading program from flash.\n");

    // Return .mpy size and location in rom
    uint32_t *_mpy_size = ((uint32_t *) &__user_flash_start) + 1;
    *buf = (uint8_t *)(_mpy_size + 1);
    return *_mpy_size;
}
#else // PYBRICKS_MPY_MAIN_MODULE

// Get user program via serial/bluetooth
static uint32_t get_user_program(uint8_t **buf) {
    pbio_error_t err;

    // Get the program length
    uint8_t len_buf[4];
    err = get_message(len_buf, 4, true, -1);
    if (err != PBIO_SUCCESS) {
        return 0;
    }

    // Convert to uint32
    uint32_t len = ((uint32_t) len_buf[0]) << 24 |
                   ((uint32_t) len_buf[1]) << 16 |
                   ((uint32_t) len_buf[2]) << 8 |
                   ((uint32_t) len_buf[3]);

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

    *buf = mpy;
    return len;
}
#endif // PYBRICKS_MPY_MAIN_MODULE

static void run_user_program(uint32_t len, uint8_t *buf) {

    if (len == 0) {
        mp_print_str(&mp_plat_print, ">>>> ERROR\n");
        return;
    }

    if (len == 4 && !strcmp((char *) buf, "REPL")) {
        m_free(buf);
        #if MICROPY_ENABLE_COMPILER
        pyexec_friendly_repl();
        #else
        mp_print_str(&mp_plat_print, "Not supported!\n");
        #endif // MICROPY_ENABLE_COMPILER
        return;
    }

    #ifdef PYBRICKS_MPY_MAIN_MODULE
    uint32_t free_len = 0;
    #else
    uint32_t free_len = len;
    #endif

    mp_reader_t reader;
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
    PB_FROM_MODULE_IMPORT_ALL(MP_QSTR_devices);
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

    // Send a message to say hub is idle
    mp_print_str(&mp_plat_print, ">>>> IDLE\n");

    // Receive an mpy-cross compiled Python script
    uint8_t *program;
    uint32_t len = get_user_program(&program);

    // If we have no bluetooth, make a fake message
    // that would otherwise be sent by the IDE to get
    // the hub into REPL. We can delete this once all
    // stm32 hubs have bluetooth enabled. Then we can
    // Use the IDE to send this message instead.
    #ifndef PYBRICKS_MPY_MAIN_MODULE
    #if !PBDRV_CONFIG_BLUETOOTH
    // Mimic the otherwise dynamic read since we free it later
    len = 4;
    program = m_malloc(len);
    program[0] = 'R';
    program[1] = 'E';
    program[2] = 'P';
    program[3] = 'L';
    #endif
    #endif

    // Get system hardware ready
    pbsys_prepare_user_program(&user_program_callbacks);

    // Initialize MicroPython and run default imports
    mp_init();
    pb_imports();

    // Send a message to say we will run a program
    mp_print_str(&mp_plat_print, "\n>>>> RUNNING\n");

    // Execute the user script
    run_user_program(len, program);

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
