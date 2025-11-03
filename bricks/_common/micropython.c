// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// This file provides a MicroPython runtime to run code in MULTI_MPY_V6 format.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/stack.h>

#include <pbio/button.h>
#include <pbio/main.h>
#include <pbio/os.h>
#include <pbio/util.h>
#include <pbio/protocol.h>
#include <pbsys/main.h>
#include <pbsys/program_stop.h>
#include <pbsys/storage.h>

#include <pybricks/common.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include "genhdr/mpversion.h"
#include "shared/readline/readline.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/interrupt_char.h"
#include "shared/runtime/pyexec.h"
#include "py/builtin.h"
#include "py/compile.h"
#include "py/frozenmod.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/objmodule.h"
#include "py/persistentcode.h"
#include "py/reader.h"
#include "py/repl.h"
#include "py/runtime.h"
#include "py/smallint.h"
#include "py/stackctrl.h"
#include "py/stream.h"

// callback for when stop button is pressed in IDE or on hub
void pbsys_main_stop_program(bool force_stop) {
    if (force_stop) {
        mp_sched_vm_abort();
    } else {
        static mp_obj_exception_t system_exit;
        system_exit.base.type = &mp_type_SystemExit;
        system_exit.traceback_alloc = system_exit.traceback_len = 0;
        system_exit.traceback_data = NULL;
        system_exit.args = (mp_obj_tuple_t *)&mp_const_empty_tuple_obj;

        mp_sched_exception(MP_OBJ_FROM_PTR(&system_exit));
    }
}

bool pbsys_main_stdin_event(uint8_t c) {
    if (c == mp_interrupt_char) {
        mp_sched_keyboard_interrupt();
        return true;
    }

    return false;
}

// Prints the exception that ended the program.
static void print_final_exception(mp_obj_t exc, int ret) {

    // Ensure exception prints on new line.
    mp_hal_stdout_tx_flush();

    nlr_buf_t nlr;
    nlr.ret_val = NULL;

    if (nlr_push(&nlr) == 0) {
        // Handle graceful stop with button.
        if ((ret & PYEXEC_FORCED_EXIT) &&
            mp_obj_exception_match(exc, MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            mp_printf(&mp_plat_print, "The program was stopped (%q).\n",
                ((mp_obj_exception_t *)MP_OBJ_TO_PTR(exc))->base.type->name);
            return;
        }

        // REVISIT: flash the light red a few times to indicate an unhandled exception?

        // Print unhandled exception with traceback.
        mp_obj_print_exception(&mp_plat_print, exc);

        nlr_pop();
    } else {
        // If we couldn't print the exception, just return. There is nothing
        // else we can do.

        // REVISIT: flash the light with a different pattern here?
    }
}

#if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL
static void run_repl(void) {
    int ret = 0;

    readline_init0();

    nlr_buf_t nlr;
    nlr.ret_val = NULL;

    if (nlr_push(&nlr) == 0) {
        nlr_set_abort(&nlr);
        // No need to set interrupt_char here, it is done by pyexec.
        #if PYBRICKS_OPT_RAW_REPL
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            // Compatibility with mpremote.
            mp_printf(&mp_plat_print, "MPY: soft reboot\n");
            ret = pyexec_raw_repl();
        } else {
            ret = pyexec_friendly_repl();
        }
        #else // PYBRICKS_OPT_RAW_REPL
        ret = pyexec_friendly_repl();
        #endif // PYBRICKS_OPT_RAW_REPL
        nlr_pop();
    } else {
        // if vm abort
        if (nlr.ret_val == NULL) {
            // we are shutting down, so don't bother with cleanup
            return;
        }

        // clear any pending exceptions (and run any callbacks).
        mp_handle_pending(false);
        // Print which exception triggered this.
        print_final_exception(MP_OBJ_FROM_PTR(nlr.ret_val), ret);
    }

    nlr_set_abort(NULL);
}
#endif // PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL


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

    for (mpy_info_t *info = mpy_first; (uintptr_t)info + sizeof(uint32_t) < (uintptr_t)mpy_end;
         info = (mpy_info_t *)(mpy_data_get_buf(info) + pbio_get_uint32_le(info->mpy_size))) {
        if (strcmp(info->mpy_name, name_str) == 0) {
            return info;
        }
    }

    return NULL;
}

// From micropython/py/builtinimport.c, but copied because it is static.
static void do_execute_proto_fun(const mp_module_context_t *context, mp_proto_fun_t proto_fun) {

    // execute the module in its context
    mp_obj_dict_t *mod_globals = context->module.globals;

    // save context
    nlr_jump_callback_node_globals_locals_t ctx;
    ctx.globals = mp_globals_get();
    ctx.locals = mp_locals_get();

    // set new context
    mp_globals_set(mod_globals);
    mp_locals_set(mod_globals);

    // set exception handler to restore context if an exception is raised
    nlr_push_jump_callback(&ctx.callback, mp_globals_locals_set_from_nlr_jump_callback);

    // make and execute the function
    mp_obj_t module_fun = mp_make_function_from_proto_fun(proto_fun, context, NULL);
    mp_call_function_0(module_fun);

    // deregister exception handler and restore context
    nlr_pop_jump_callback(true);
}

static void execute_rom_mpy_in_context(mp_module_context_t *module_context, mpy_info_t *mpy_info) {
    // Prepare to execute in its own context.
    mp_compiled_module_t compiled_module;
    compiled_module.context = module_context;

    // Execute the MPY file in the new module context.
    mp_reader_t reader;
    mp_reader_new_mem(&reader, mpy_data_get_buf(mpy_info), pbio_get_uint32_le(mpy_info->mpy_size), MP_READER_IS_ROM);
    mp_raw_code_load(&reader, &compiled_module);
    do_execute_proto_fun(compiled_module.context, compiled_module.rc);
}

/**
 * Runs the __main__ module from user RAM.
 */
static void run_user_program(void) {
    int ret = 0;

    nlr_buf_t nlr;
    nlr.ret_val = NULL;

    if (nlr_push(&nlr) == 0) {
        nlr_set_abort(&nlr);

        // Run the script while letting CTRL-C interrupt it.
        mp_hal_set_interrupt_char(CHAR_CTRL_C);

        // Main program runs in __main__ module which is already initialized.
        mp_module_context_t main_context = {
            .module = mp_module___main__
        };
        execute_rom_mpy_in_context(&main_context, mpy_first);

        mp_hal_set_interrupt_char(-1);

        // Handle any pending exceptions (and any callbacks)
        mp_handle_pending(true);

        nlr_pop();
    } else {
        mp_hal_set_interrupt_char(-1);

        // if vm abort
        if (nlr.ret_val == NULL) {
            // we are shutting down, so don't bother with cleanup
            return;
        }

        // Clear any pending exceptions (and run any callbacks).
        mp_handle_pending(false);

        if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t *)nlr.ret_val)->type), MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            // at the moment, the value of SystemExit is unused
            ret = PYEXEC_FORCED_EXIT;
        }

        print_final_exception(MP_OBJ_FROM_PTR(nlr.ret_val), ret);

        #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL
        // On KeyboardInterrupt, drop to REPL for debugging.
        if (mp_obj_exception_match(MP_OBJ_FROM_PTR(nlr.ret_val), MP_OBJ_FROM_PTR(&mp_type_KeyboardInterrupt))) {

            // The global scope is preserved to facilitate debugging, but we
            // stop active resources like motors and sounds. They are stopped
            // but not reset so the user can restart them in the REPL.
            pbio_main_soft_stop();

            // Enter REPL.
            run_repl();
        }
        #endif // PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL
    }

    nlr_set_abort(NULL);
}

pbio_error_t pbsys_main_program_validate(pbsys_main_program_t *program) {

    // For builtin programs, check requested ID against feature flags.
    #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL
    if (program->id == PBIO_PYBRICKS_USER_PROGRAM_ID_REPL) {
        return PBIO_SUCCESS;
    }
    #endif
    #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_PORT_VIEW
    if (program->id == PBIO_PYBRICKS_USER_PROGRAM_ID_PORT_VIEW) {
        return PBIO_SUCCESS;
    }
    #endif
    #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_IMU_CALIBRATION
    if (program->id == PBIO_PYBRICKS_USER_PROGRAM_ID_IMU_CALIBRATION) {
        return PBIO_SUCCESS;
    }
    #endif

    // If requesting a user program, ensure that it exists and is valid.
    uint32_t program_size = program->code_end - program->code_start;
    if (program_size == 0 || program_size > pbsys_storage_get_maximum_program_size()) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    // Application-specific name may be used in system UI.
    mpy_info_t *mpy_info = (mpy_info_t *)program->code_start;
    program->name = mpy_info->mpy_name;

    // This is the same test done when loading the mpy. Do it early so we don't
    // start MicroPython and so the system knows this is valid. Revisit: Consider
    // making this part of the public API upstream.
    uint8_t *header = mpy_data_get_buf(mpy_info);
    uint8_t arch = MPY_FEATURE_DECODE_ARCH(header[2]);
    if (header[0] != 'M'
        || header[1] != MPY_VERSION
        || (arch != MP_NATIVE_ARCH_NONE && MPY_FEATURE_DECODE_SUB_VERSION(header[2]) != MPY_SUB_VERSION)
        || header[3] > MP_SMALL_INT_BITS) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return PBIO_SUCCESS;
}

const char *pbsys_main_get_application_version_hash(void) {
    // This is (somewhat confusingly) passed in as the MICROPY_GIT_HASH.
    // REVISIT: Make PYBRICKS_GIT_HASH available in a pbio header via a build step.
    return MICROPY_GIT_HASH;
}

// Runs MicroPython with the given program data.
void pbsys_main_run_program(pbsys_main_program_t *program) {

    #if PBDRV_CONFIG_STACK_EMBEDDED
    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    char *stack_start;
    char *stack_end;
    pbdrv_stack_get_info(&stack_start, &stack_end);
    #if PYBRICKS_OPT_USE_STACK_END_AS_TOP
    mp_stack_set_top(stack_end);
    #else
    // Sets the top to current stack pointer.
    mp_stack_ctrl_init();
    #endif
    mp_stack_set_limit(MP_STATE_THREAD(stack_top) - stack_start - 1024);
    #else
    mp_cstack_init_with_sp_here(1024 * 1024);
    #endif

    // MicroPython heap is the free RAM after program data.
    gc_init(program->user_ram_start, program->user_ram_end);

    // Set program data reference to first script. This is used to run main,
    // and to set the starting point for finding downloaded modules.
    mpy_data_init(program);

    // Initialize MicroPython.
    mp_init();

    // Runs the requested downloaded or builtin user program.
    switch (program->id) {

        #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_REPL
        case PBIO_PYBRICKS_USER_PROGRAM_ID_REPL:
            // Run REPL with everything auto-imported.
            pb_package_pybricks_init(true);
            run_repl();
            break;
        #endif

        #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_PORT_VIEW && MICROPY_MODULE_FROZEN
        case PBIO_PYBRICKS_USER_PROGRAM_ID_PORT_VIEW:
            pb_package_pybricks_init(false);
            pyexec_frozen_module("_builtin_port_view.py", false);
            break;
        #endif

        #if PBSYS_CONFIG_FEATURE_BUILTIN_USER_PROGRAM_IMU_CALIBRATION
        case PBIO_PYBRICKS_USER_PROGRAM_ID_IMU_CALIBRATION:
            // Todo
            break;
        #endif

        default:
            // Init Pybricks package without auto-import.
            pb_package_pybricks_init(false);
            // Run loaded user program (just slot 0 for now).
            run_user_program();
            break;
    }

    // Ensure everything is written before the user application is considered
    // done, so that the host does not receive stdout after receiving stop.
    mp_hal_stdout_tx_flush();
}

void pbsys_main_run_program_cleanup(void) {
    gc_sweep_all();
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

    // Check if the module is already loaded.
    mp_map_elem_t *elem = mp_map_lookup(&MP_STATE_VM(mp_loaded_modules_dict).map, args[0], MP_MAP_LOOKUP);
    if (elem) {
        return elem->value;
    }

    // Try the name directly as a non-extensible built-in (e.g. `micropython`).
    qstr module_name_qstr = mp_obj_str_get_qstr(args[0]);
    mp_obj_t module_obj = mp_module_get_builtin(module_name_qstr, false);
    if (module_obj != MP_OBJ_NULL) {
        return module_obj;
    }

    // Now try as an extensible built-in (e.g. `struct`/`ustruct`).
    module_obj = mp_module_get_builtin(module_name_qstr, true);
    if (module_obj != MP_OBJ_NULL) {
        return module_obj;
    }

    // Check for presence of user program in user RAM.
    mpy_info_t *info = mpy_data_find(module_name_qstr);

    // If a downloaded module was found but not yet loaded, load it.
    if (info) {
        // Create new module.
        mp_module_context_t *module_context = mp_obj_new_module(module_name_qstr);

        // Execute the module in that context.
        execute_rom_mpy_in_context(module_context, info);

        // Return the newly imported module.
        return MP_OBJ_FROM_PTR(module_context);
    }

    // Allow importing of frozen modules if any were included in the firmware.
    #if MICROPY_MODULE_FROZEN_MPY
    void *modref;
    int frozen_type;
    const char *ext = ".py";
    char module_path[(1 << (8 * MICROPY_QSTR_BYTES_IN_LEN)) + sizeof(ext)] = { 0 };
    strcpy(module_path, mp_obj_str_get_str(args[0]));
    strcpy(module_path + qstr_len(module_name_qstr), ext);
    if (mp_find_frozen_module(module_path, &frozen_type, &modref) == MP_IMPORT_STAT_FILE && frozen_type == MP_FROZEN_MPY) {
        // Create new module to be returned.
        mp_module_context_t *module_context = mp_obj_new_module(module_name_qstr);
        mp_obj_t module_obj = MP_OBJ_FROM_PTR(module_context);

        // Execute frozen code in the new module context.
        const mp_frozen_module_t *frozen = modref;
        module_context->constants = frozen->constants;
        do_execute_proto_fun(module_context, frozen->proto_fun);
        return module_obj;
    }
    #endif

    // Nothing found, raise ImportError.
    mp_raise_msg_varg(&mp_type_ImportError, MP_ERROR_TEXT("no module named '%q'"), module_name_qstr);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
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
