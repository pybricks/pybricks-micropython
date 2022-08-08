// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbsys/main.h>
#include <pbsys/user_program.h>

#include <pybricks/common.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_flash.h>

#include "shared/readline/readline.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/interrupt_char.h"
#include "shared/runtime/pyexec.h"
#include "py/builtin.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/persistentcode.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/stream.h"

// defined in linker script
extern uint32_t _estack;
extern uint32_t _sstack;
extern uint32_t _heap_start;
extern uint32_t _heap_end;

// Implementation for MICROPY_EVENT_POLL_HOOK
void pb_stm32_poll(void) {
    while (pbio_do_one_event()) {
    }

    mp_handle_pending(true);

    // There is a possible race condition where an interrupt occurs and sets the
    // Contiki poll_requested flag after all events have been processed. So we
    // have a critical section where we disable interrupts and check see if there
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
static pbio_error_t get_message(uint8_t *buf, uint32_t rx_len, int time_out) {
    // Maximum time between two bytes/chunks
    const mp_uint_t time_interval = 500;

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
            if (time_out >= 0 && time_now - time_start > (mp_uint_t)time_out) {
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

    // Set max program size as half of heap for now.
    const uint32_t max_mpy_size = ((char *)&_heap_end - (char *)&_heap_start) / 2;

    // flush any buffered bytes from stdin
    while (mp_hal_stdio_poll(MP_STREAM_POLL_RD)) {
        mp_hal_stdin_rx_chr();
    }

    // Get the program length
    uint32_t len;
    err = get_message((uint8_t *)&len, sizeof(len), -1);

    // If button was pressed, return code to run script in flash
    if (err == PBIO_ERROR_CANCELED) {
        #if (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)
        // Open existing main.mpy file from flash
        uint32_t size = 0;
        if (pb_flash_file_open_get_size("/_pybricks/main.mpy", &size) != PBIO_SUCCESS) {
            return 0;
        }
        // Check size and allocate buffer

        if (size > max_mpy_size) {
            return 0;
        }
        *buf = m_malloc(size);
        if (*buf == NULL) {
            return 0;
        }
        // Read the file contents
        if (pb_flash_file_read(*buf, size) != PBIO_SUCCESS) {
            m_free(*buf);
            return 0;
        }
        *free_len = size;
        return size;
        #else
        // Load main program embedded in firmware
        *buf = &_pb_user_mpy_data;
        return _pb_user_mpy_size;
        #endif
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
    if (len > max_mpy_size) {
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

    #if (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)
    // Save program as file
    pb_flash_file_write("/_pybricks/main.mpy", *buf, len);
    #endif // (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)

    return len;
}

// callback for when stop button is pressed in IDE or on hub
static void user_program_stop_func(void) {
    static const mp_obj_tuple_t args = {
        .base = { .type = &mp_type_tuple },
        .len = 1,
        .items = { MP_ROM_QSTR(MP_QSTR_stop_space_button_space_pressed) },
    };
    static mp_obj_exception_t system_exit;

    // Trigger soft reboot.
    pyexec_system_exit = PYEXEC_FORCED_EXIT;

    // Schedule SystemExit exception.
    system_exit.base.type = &mp_type_SystemExit;
    system_exit.traceback_alloc = 0;
    system_exit.traceback_len = 0;
    system_exit.traceback_data = NULL;
    system_exit.args = (mp_obj_tuple_t *)&args;
    MP_STATE_MAIN_THREAD(mp_pending_exception) = MP_OBJ_FROM_PTR(&system_exit);
    #if MICROPY_ENABLE_SCHEDULER
    if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
        MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
    }
    #endif
}

static bool user_program_stdin_event_func(uint8_t c) {
    if (c == mp_interrupt_char) {
        mp_sched_keyboard_interrupt();
        return true;
    }

    return false;
}

static const pbsys_user_program_callbacks_t user_program_callbacks = {
    .stop = user_program_stop_func,
    .stdin_event = user_program_stdin_event_func,
};

static void run_user_program(uint32_t len, uint8_t *buf, uint32_t free_len) {
    bool run_repl = len == REPL_LEN;

    #if MICROPY_ENABLE_COMPILER
    bool import_all = run_repl;
    #endif

restart:
    // Hook into pbsys
    pbsys_user_program_prepare(&user_program_callbacks);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        // Get all builtin modules ready for use.
        pb_package_pybricks_init();

        if (run_repl) {
            #if MICROPY_ENABLE_COMPILER
            // If the user requested the REPL without a user program, import all
            // pybricks packages for convience.
            if (import_all) {
                pb_package_import_all();
            }
            pyexec_friendly_repl();
            #else
            mp_hal_stdout_tx_str("REPL not supported!\r\n");
            #endif // MICROPY_ENABLE_COMPILER
        } else {
            // run user .mpy file
            mp_reader_t reader;
            mp_reader_new_mem(&reader, buf, len, free_len);
            mp_module_context_t *context = m_new_obj(mp_module_context_t);
            context->module.globals = mp_globals_get();
            mp_compiled_module_t compiled_module = mp_raw_code_load(&reader, context);
            mp_obj_t module_fun = mp_make_function_from_raw_code(compiled_module.rc, context, MP_OBJ_NULL);
            mp_hal_set_interrupt_char(CHAR_CTRL_C); // allow ctrl-C to interrupt us
            mp_call_function_0(module_fun);
            mp_hal_set_interrupt_char(-1); // disable interrupt
            mp_handle_pending(true); // handle any pending exceptions (and any callbacks)
        }
        nlr_pop();
    } else {
        // uncaught exception
        mp_hal_set_interrupt_char(-1); // disable interrupt
        mp_handle_pending(false); // clear any pending exceptions (and run any callbacks)

        // Need to unprepare, otherwise SystemExit could be raised during print.
        pbsys_user_program_unprepare();
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        // If there was KeyboardInterrupt in the user program, drop to REPL
        // for debugging. If the REPL was already running, exit.
        if (!run_repl && mp_obj_exception_match((mp_obj_t)nlr.ret_val, &mp_type_KeyboardInterrupt)) {
            run_repl = true;
            goto restart;
        }
    }

    // Clean up resources that may have been used by pybricks package.
    pb_package_pybricks_deinit();
    pbsys_user_program_unprepare();
}

void pbsys_main_application(void) {

    #if (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)
    mp_hal_delay_ms(500);
    pb_flash_init();
    #endif

soft_reset:
    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);

    #if MICROPY_ENABLE_GC
    gc_init(&_heap_start, &_heap_end);
    #endif

    wait_for_button_release();

    // Receive an mpy-cross compiled Python script
    uint8_t *program;
    uint32_t free_len;
    uint32_t len = get_user_program(&program, &free_len);

    mp_init();

    // Execute the user script
    run_user_program(len, program, free_len);

    // Uninitialize MicroPython and the system hardware
    mp_deinit();

    goto soft_reset;

    MP_UNREACHABLE;
}

void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

void mp_reader_new_file(mp_reader_t *reader, const char *filename) {
    if (strcmp(filename, "main.mpy") != 0) {
        mp_raise_OSError(MP_ENOENT);
    }

    mp_reader_new_mem(reader, &_pb_user_mpy_data, _pb_user_mpy_size, 0);
}

mp_import_stat_t mp_import_stat(const char *path) {
    if (strcmp(path, "main.mpy") == 0) {
        return MP_IMPORT_STAT_FILE;
    }

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
