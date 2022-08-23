// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <contiki.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbsys/main.h>
#include <pbsys/program_stop.h>

#include <pybricks/common.h>
#include <pybricks/util_mp/pb_obj_helper.h>

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

// callback for when stop button is pressed in IDE or on hub
void pbsys_main_stop_program(void) {

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

bool pbsys_main_stdin_event(uint8_t c) {
    if (c == mp_interrupt_char) {
        mp_sched_keyboard_interrupt();
        return true;
    }

    return false;
}

// The following defines a reader for use by micropython/py/persistentcode.c.
typedef struct _mp_vfs_map_minimal_t {
    const byte *cur;
    const byte *end;
} mp_vfs_map_minimal_t;

mp_uint_t mp_vfs_map_minimal_readbyte(void *data) {
    mp_vfs_map_minimal_t *blob = (mp_vfs_map_minimal_t *)data;
    return (blob->cur < blob->end) ? *blob->cur++ : MP_READER_EOF;
}

const uint8_t *mp_vfs_map_minimal_read_bytes(mp_reader_t *reader, size_t len) {
    mp_vfs_map_minimal_t *blob = (mp_vfs_map_minimal_t *)reader->data;
    const uint8_t *ptr = blob->cur;
    blob->cur += len;
    return ptr;
}

static void mp_vfs_map_minimal_close(void *data) {
}

static void mp_vfs_map_minimal_new_reader(mp_reader_t *reader, mp_vfs_map_minimal_t *data, const byte *buf, size_t len) {
    data->cur = buf;
    data->end = buf + len;
    reader->data = data;
    reader->readbyte = mp_vfs_map_minimal_readbyte;
    reader->close = mp_vfs_map_minimal_close;
}

static void run_repl() {
    #if MICROPY_ENABLE_COMPILER
    // Reset REPL history.
    readline_init0();
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // Run the REPL.
        pyexec_friendly_repl();
        nlr_pop();
    } else {
        // clear any pending exceptions (and run any callbacks).
        mp_handle_pending(false);
        // Print which exception triggered this.
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
    #else
    mp_hal_stdout_tx_str("REPL not supported!\r\n");
    #endif
}

static void run_user_program(uint32_t len, uint8_t *buf) {

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        // Load user .mpy file without erasing it.
        mp_reader_t reader;
        mp_vfs_map_minimal_t data;
        mp_vfs_map_minimal_new_reader(&reader, &data, buf, len);
        mp_module_context_t *context = m_new_obj(mp_module_context_t);
        context->module.globals = mp_globals_get();
        mp_compiled_module_t compiled_module = mp_raw_code_load(&reader, context);
        mp_obj_t module_fun = mp_make_function_from_raw_code(compiled_module.rc, context, MP_OBJ_NULL);

        // Run the script while letting CTRL-C interrupt it.
        mp_hal_set_interrupt_char(CHAR_CTRL_C);
        mp_call_function_0(module_fun);
        mp_hal_set_interrupt_char(-1);

        // Handle any pending exceptions (and any callbacks)
        mp_handle_pending(true);

        nlr_pop();
    } else {
        // Clear any pending exceptions (and run any callbacks).
        mp_hal_set_interrupt_char(-1);
        mp_handle_pending(false);

        // Print which exception triggered this.
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);

        // On KeyboardInterrupt, drop to REPL for debugging.
        if (mp_obj_exception_match((mp_obj_t)nlr.ret_val, &mp_type_KeyboardInterrupt)) {

            // The global scope is preserved to facilitate debugging, but we
            // stop active resources like motors and sounds. They are stopped
            // but not reset so the user can restart them in the REPL.
            pbio_stop_all(false);

            // Enter REPL.
            run_repl();
        }
    }
}

// Runs MicroPython with the given program data.
void pbsys_main_run_program(pbsys_main_program_t *program) {

    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);

    // MicroPython heap starts after program data.
    gc_init(program->data + program->size, &_heap_end);

    // Initialize MicroPython.
    mp_init();

    // For MicroPython, the builtin program is the REPL.
    if (program->run_builtin) {
        // Init Pybricks package and auto-import everything.
        pb_package_pybricks_init(true);

        // Start the REPL.
        run_repl();
    } else {
        // Init Pybricks package without auto-import.
        pb_package_pybricks_init(false);

        // Execute the user script.
        run_user_program(program->size, program->data);
    }

    // Clean up non-MicroPython resources used by the pybricks package.
    pb_package_pybricks_deinit();

    // Uninitialize MicroPython.
    mp_deinit();
}

void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
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
