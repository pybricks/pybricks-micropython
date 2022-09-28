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
#include "py/objmodule.h"
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

// From micropython/py/builtinimport.c, but copied because it is static.
static void do_execute_raw_code(mp_module_context_t *context, const mp_raw_code_t *rc, const mp_module_context_t *mc) {

    // execute the module in its context
    mp_obj_dict_t *mod_globals = context->module.globals;

    // save context
    mp_obj_dict_t *volatile old_globals = mp_globals_get();
    mp_obj_dict_t *volatile old_locals = mp_locals_get();

    // set new context
    mp_globals_set(mod_globals);
    mp_locals_set(mod_globals);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t module_fun = mp_make_function_from_raw_code(rc, mc, NULL);
        mp_call_function_0(module_fun);

        // finish nlr block, restore context
        nlr_pop();
        mp_globals_set(old_globals);
        mp_locals_set(old_locals);
    } else {
        // exception; restore context and re-raise same exception
        mp_globals_set(old_globals);
        mp_locals_set(old_locals);
        nlr_jump(nlr.ret_val);
    }
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

/** mpy info and data for one script or module. */
typedef struct {
    /** Size of the mpy program. */
    uint32_t mpy_size;
    /** Null-terminated name of the script, without file extension. */
    char mpy_name[];
    /** mpy data follows thereafter. */
} mpy_info_t;

// Gets a reference to the mpy data of a script.
static uint8_t *mpy_data_get_buf(mpy_info_t *info) {
    return (uint8_t *)info + sizeof(info->mpy_size) + strlen(info->mpy_name) + 1;
}

// Program data is a concatenation of multiple mpy files. This sets a reference
// to the first script and the total size so we can search for modules.
static mpy_info_t *mpy_first;
static uint32_t mpy_size_total;
static inline void mpy_data_init(pbsys_main_program_t *program) {
    mpy_first = (mpy_info_t *)program->data;
    mpy_size_total = program->size;
}

// Finds a MicroPython module in the program data.
static mpy_info_t *mpy_data_find(const char *name, uint32_t leading_dots) {

    // Start at first script.
    mpy_info_t *next = mpy_first;

    // Iterate through the programs while not found.
    while (strcmp(next->mpy_name + leading_dots, name)) {
        // Exit if we're passed the end.
        if ((uint8_t *)next >= (uint8_t *)mpy_first + mpy_size_total) {
            return NULL;
        }
        // Get reference to next script.
        next = (mpy_info_t *)(mpy_data_get_buf(next) + next->mpy_size);
    }
    return next;
}

// Runs MicroPython with the given program data.
void pbsys_main_run_program(pbsys_main_program_t *program) {

    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);

    // MicroPython heap starts after program data, aligned by GC block size.
    uint32_t align = MICROPY_BYTES_PER_GC_BLOCK -
        ((uint32_t)(program->data) + program->size) % MICROPY_BYTES_PER_GC_BLOCK;
    gc_init(program->data + program->size + align, &_heap_end);

    // Set program data reference to first script. This is used to run main,
    // and to set the starting point for finding downloaded modules.
    mpy_data_init(program);

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

        // For backwards compatibility, detect old-format single-script data.
        if (program->data[0] == 'M') {
            // TODO: This case be removed once relevant IDE tools are updated.
            run_user_program(program->size, program->data);
        } else {
            // Execute the first script, which is the main script.
            run_user_program(mpy_first->mpy_size, mpy_data_get_buf(mpy_first));
        }
    }

    // Clean up non-MicroPython resources used by the pybricks package.
    pb_package_pybricks_deinit();

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

// Overrides MicroPython's mp_builtin___import__
mp_obj_t pb_builtin_import(size_t n_args, const mp_obj_t *args) {

    // For backwards compatibility, use default import for legacy scripts.
    // TODO: This case can be removed once relevant IDE tools are updated.
    if (((uint8_t *)mpy_first)[0] == 'M') {
        return mp_builtin___import___default(n_args, args);
    }

    // Check for relative imports
    uint32_t leading_dots = n_args < 5 ? 0 : MP_OBJ_SMALL_INT_VALUE(args[4]);

    // Check for presence in downloaded modules. It is normally faster to scan
    // for imported modules first, but if we find it, we can grab the ready-
    // made static qstring instead of assembling strings with leading dots.
    mpy_info_t *info = mpy_data_find(mp_obj_str_get_str(args[0]), leading_dots);

    qstr module_name_qstr;
    if (info) {
        // If found, get qstr from downloaded module, including leading dots.
        module_name_qstr = qstr_from_strn_static(info->mpy_name, strlen(info->mpy_name));
    } else {
        // Otherwise get it directly from the argument. This has no leading
        // dots, but builtin relative modules don't exist anyway.
        module_name_qstr = mp_obj_str_get_qstr(args[0]);
    }

    // Check if module already exists, and return it if it does
    mp_obj_t module_obj = mp_module_get_loaded_or_builtin(module_name_qstr);
    if (module_obj != MP_OBJ_NULL) {
        return module_obj;
    }

    // If a downloaded module was found but not yet loaded, load it.
    if (info) {
        // Parse the static script data.
        mp_reader_t reader;
        mp_vfs_map_minimal_t data;
        mp_vfs_map_minimal_new_reader(&reader, &data, mpy_data_get_buf(info), info->mpy_size);

        // Create new module and execute in its own context.
        mp_obj_t module_obj = mp_obj_new_module(module_name_qstr);
        mp_module_context_t *context = MP_OBJ_TO_PTR(module_obj);
        mp_compiled_module_t compiled_module = mp_raw_code_load(&reader, context);
        do_execute_raw_code(context, compiled_module.rc, compiled_module.context);

        // Return the newly imported module.
        return module_obj;
    }

    // Nothing found, raise ImportError.
    mp_raise_msg_varg(&mp_type_ImportError, MP_ERROR_TEXT("no module named '%q'"), module_name_qstr);
}

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
