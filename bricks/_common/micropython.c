// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

// This file provides a MicroPython runtime to run code in MULTI_MPY_V6 format.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/util.h>
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

// Implementation for MICROPY_EVENT_POLL_HOOK
void pb_event_poll_hook(void) {
    while (pbio_do_one_event()) {
    }

    mp_handle_pending(true);

    // Platform-specific code to run on completing the poll hook.
    pb_event_poll_hook_leave();
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

#if PYBRICKS_OPT_COMPILER
static void run_repl() {
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
}
#endif

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

/** mpy info and data for one script or module. */
typedef struct {
    /** Size of the mpy program. Not aligned, use pbio_get_uint32_le() to read. */
    uint8_t mpy_size[4];
    /** Null-terminated name of the script, without file extension. */
    char mpy_name[];
    /** mpy data follows thereafter. */
} mpy_info_t;

// Program data is a concatenation of multiple mpy files. This sets a reference
// to the first script and the total size so we can search for modules.
static mpy_info_t *mpy_first;
static mpy_info_t *mpy_end;
static inline void mpy_data_init(pbsys_main_program_t *program) {
    mpy_first = (mpy_info_t *)program->code_start;
    mpy_end = (mpy_info_t *)program->code_end;
}

/**
 * Gets a reference to the mpy data of a script.
 * @param [in]  info    A pointer to an mpy info header.
 * @return              A pointer to the .mpy file.
 */
static uint8_t *mpy_data_get_buf(mpy_info_t *info) {
    // The header consists of the size and a zero-terminated module name string.
    return (uint8_t *)info + sizeof(info->mpy_size) + strlen(info->mpy_name) + 1;
}

/**
 * Finds a MicroPython module in the program data.
 * @param [in]  name    The fully qualified name of the module.
 * @return              A pointer to the .mpy file in user RAM or NULL if the
 *                      module was not found.
 */
static mpy_info_t *mpy_data_find(qstr name) {
    const char *name_str = qstr_str(name);

    for (mpy_info_t *info = mpy_first; info < mpy_end;
         info = (mpy_info_t *)(mpy_data_get_buf(info) + pbio_get_uint32_le(info->mpy_size))) {
        if (strcmp(info->mpy_name, name_str) == 0) {
            return info;
        }
    }

    return NULL;
}

/**
 * Runs the __main__ module from user RAM.
 */
static void run_user_program(void) {

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mpy_info_t *info = mpy_data_find(MP_QSTR___main__);

        if (!info) {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("no __main__ module"));
        }

        // This is similar to __import__ except we don't push/pop globals
        mp_reader_t reader;
        mp_vfs_map_minimal_t data;
        mp_vfs_map_minimal_new_reader(&reader, &data, mpy_data_get_buf(info), pbio_get_uint32_le(info->mpy_size));
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

        #if PYBRICKS_OPT_COMPILER
        // On KeyboardInterrupt, drop to REPL for debugging.
        if (mp_obj_exception_match((mp_obj_t)nlr.ret_val, &mp_type_KeyboardInterrupt)) {

            // The global scope is preserved to facilitate debugging, but we
            // stop active resources like motors and sounds. They are stopped
            // but not reset so the user can restart them in the REPL.
            pbio_stop_all(false);

            // Enter REPL.
            run_repl();
        }
        #endif
    }
}

// Runs MicroPython with the given program data.
void pbsys_main_run_program(pbsys_main_program_t *program) {

    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    char *sstack;
    char *estack;
    pb_stack_get_info(&sstack, &estack);
    mp_stack_set_top(estack);
    mp_stack_set_limit(estack - sstack - 1024);

    // MicroPython heap starts after program data, aligned by GC block size.
    uint32_t align = MICROPY_BYTES_PER_GC_BLOCK -
        (uint32_t)program->code_end % MICROPY_BYTES_PER_GC_BLOCK;
    gc_init(program->code_end + align, program->data_end);

    // Set program data reference to first script. This is used to run main,
    // and to set the starting point for finding downloaded modules.
    mpy_data_init(program);

    // Initialize MicroPython.
    mp_init();

    // Check for run type.
    if (!program->run_builtin) {
        // Init Pybricks package without auto-import.
        pb_package_pybricks_init(false);
        // Run loaded program.
        run_user_program();
    }
    #if PYBRICKS_OPT_COMPILER
    else {
        // For MicroPython, the builtin program is the REPL.
        // Run it with everything auto-imported.
        pb_package_pybricks_init(true);
        run_repl();
    }
    #endif // PYBRICKS_OPT_COMPILER

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
// IMPORTANT: this needs to be kept in sync with mp_builtin___import___default().
mp_obj_t pb_builtin_import(size_t n_args, const mp_obj_t *args) {
    // Check that it's not a relative import
    if (n_args >= 5 && MP_OBJ_SMALL_INT_VALUE(args[4]) != 0) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("relative import"));
    }

    // Check if module already exists, and return it if it does
    qstr module_name_qstr = mp_obj_str_get_qstr(args[0]);
    mp_obj_t module_obj = mp_module_get_loaded_or_builtin(module_name_qstr);
    if (module_obj != MP_OBJ_NULL) {
        return module_obj;
    }

    // Check for presence of user program in user RAM.
    mpy_info_t *info = mpy_data_find(module_name_qstr);

    // If a downloaded module was found but not yet loaded, load it.
    if (info) {
        // Parse the static script data.
        mp_reader_t reader;
        mp_vfs_map_minimal_t data;
        mp_vfs_map_minimal_new_reader(&reader, &data, mpy_data_get_buf(info), pbio_get_uint32_le(info->mpy_size));

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

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
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
