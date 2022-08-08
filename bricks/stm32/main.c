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

// REVISIT: We could modify the linker script like upstream MicroPython so that
// we can specify the stack size per hub in the linker scripts. Currently, since
// the heap is statically allocated here, the stack size is whatever is left in
// RAM after .data and .bss sections. But it would probably be better to specify
// the stack size and let the heap be whatever is leftover.

// defined in linker script
extern uint32_t _estack;
extern uint32_t _ebss;

#ifndef MICROPY_ENABLE_GC
#warning GC Should be enabled for embedded builds.
#endif

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

// MicroPython heap starts after the (down)loaded user program .mpy blob
uint8_t program_and_heap[PYBRICKS_HEAP_KB * 1024];
uint32_t program_size;

// The program can't take up the entire heap, since we still need
// a bit of heap to parse the file and run it.
const uint32_t program_size_max = sizeof(program_and_heap) * 2 / 3;

static uint32_t program_get_new_size(void) {
    // flush any buffered bytes from stdin
    while (mp_hal_stdio_poll(MP_STREAM_POLL_RD)) {
        mp_hal_stdin_rx_chr();
    }

    // Wait for program size message.
    uint32_t len;
    pbio_error_t err;
    err = get_message((uint8_t *)&len, sizeof(len), -1);

    // Did not get valid message or the button was pressed.
    // Return 0 to indicate we won't be getting a new program.
    if (err != PBIO_SUCCESS) {
        return 0;
    }

    // Return expected program length.
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

typedef struct _mp_reader_blob_t {
    const byte *beg;
    const byte *cur;
    const byte *end;
} mp_reader_blob_t;

STATIC mp_uint_t mp_reader_blob_readbyte(void *data) {
    mp_reader_blob_t *blob = (mp_reader_blob_t *)data;
    if (blob->cur < blob->end) {
        return *blob->cur++;
    } else {
        return MP_READER_EOF;
    }
}

STATIC void mp_reader_blob_close(void *data) {
    (void)data;
}

const uint8_t *mp_reader_blob_readchunk(void *data, size_t len) {
    mp_reader_blob_t *blob = (mp_reader_blob_t *)data;
    const uint8_t *ptr = blob->cur;
    blob->cur += len;
    return ptr;
}

STATIC void mp_reader_new_blob(mp_reader_t *reader, mp_reader_blob_t *blob, const byte *buf, size_t len) {
    blob->beg = buf;
    blob->cur = buf;
    blob->end = buf + len;
    reader->data = blob;
    reader->readbyte = mp_reader_blob_readbyte;
    reader->readchunk = mp_reader_blob_readchunk;
    reader->close = mp_reader_blob_close;
}

static void run_user_program(bool run_repl, uint8_t *mpy, uint32_t mpy_size) {

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
            mp_reader_blob_t blob;
            mp_reader_new_blob(&reader, &blob, mpy, mpy_size);
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

static void stm32_main(void) {

    // Load initial program from storage.
    // REVISIT: Use platform agnostic block device.
    #if (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)
    mp_hal_delay_ms(500);
    pb_flash_init();
    uint32_t size = 0;
    if (pb_flash_file_open_get_size("/_pybricks/main.mpy", &size) == PBIO_SUCCESS) {
        if (size < program_size_max && pb_flash_file_read(program_and_heap, size) == PBIO_SUCCESS) {
            program_size = size;
        }
    }
    #else
    program_size = _pb_user_mpy_size;
    memcpy(program_and_heap, &_pb_user_mpy_data, program_size);
    #endif

soft_reset:
    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_ebss - 1024);

    wait_for_button_release();

    // Get expected size of program.
    bool run_repl = false;
    uint32_t mpy_size = program_get_new_size();

    // Decide what to do based on expected size.
    if (mpy_size == REPL_LEN) {
        // REPL is indicated by a special size.
        run_repl = true;
        mpy_size = 0;
    } else if (mpy_size == 0) {
        // On zero, run the stored program again.
        mpy_size = program_size;
    } else if (mpy_size < program_size_max
               && get_message(program_and_heap, mpy_size, 500) == PBIO_SUCCESS) {
        // If the size is valid, receive and update the stored program.
        program_size = mpy_size;
    } else {
        // On failure, program space may be garbage, so set size to
        // match and try again.
        program_size = 0;
        mpy_size = 0;
    }

    // Initialize heap to start directly after received program.
    gc_init(program_and_heap + mpy_size, program_and_heap + sizeof(program_and_heap));

    mp_init();

    // Execute the user script
    run_user_program(run_repl, program_and_heap, mpy_size);

    // Uninitialize MicroPython and the system hardware
    mp_deinit();

    goto soft_reset;

    // TODO: On power off, save program data.

    MP_UNREACHABLE;
}

int main(int argc, char **argv) {
    pbsys_main(stm32_main);
    return 0;
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
