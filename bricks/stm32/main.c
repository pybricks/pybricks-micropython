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

typedef struct _pb_reader_user_data_t {
    const byte *beg;
    const byte *cur;
    const byte *end;
} pb_reader_user_data_t;

mp_uint_t pb_reader_user_data_readbyte(void *data) {
    pb_reader_user_data_t *blob = (pb_reader_user_data_t *)data;
    if (blob->cur < blob->end) {
        return *blob->cur++;
    } else {
        return MP_READER_EOF;
    }
}

static void pb_reader_user_data_close(void *data) {
    (void)data;
}

const uint8_t *pb_reader_user_data_readchunk(void *data, size_t len) {
    pb_reader_user_data_t *blob = (pb_reader_user_data_t *)data;
    const uint8_t *ptr = blob->cur;
    blob->cur += len;
    return ptr;
}

static void pb_reader_user_data_new(mp_reader_t *reader, pb_reader_user_data_t *data, const byte *buf, size_t len) {
    data->beg = buf;
    data->cur = buf;
    data->end = buf + len;
    reader->data = data;
    reader->readbyte = pb_reader_user_data_readbyte;
    reader->readchunk = pb_reader_user_data_readchunk;
    reader->close = pb_reader_user_data_close;
}

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

static void run_user_program(uint32_t len, uint8_t *buf, bool run_repl) {

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
            // run user .mpy file without erasing it
            mp_reader_t reader;
            pb_reader_user_data_t data;
            pb_reader_user_data_new(&reader, &data, buf, len);
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

// MPY info and data for one script or module.
typedef struct {
    /**
     * Size of the mpy program.
     */
    uint32_t mpy_size;
    /**
     * Null-terminated name of the script, padded with additional zeros to
     * make the size an integer multiple of 4.
     */
    char mpy_name[];
    /**
     * Data follows thereafter.
     */
}  __attribute__((scalar_storage_order("little-endian"))) mpy_info_t;

// Rounds up a given size to an integer multiple of 4.
static uint32_t padded(uint32_t len) {
    if (len % 4 == 0) {
        return len;
    }
    return len + 4 - (len % 4);
}

// Gets a reference to the mpy data of a script.
static uint8_t *mpy_data_get_buf(mpy_info_t *info) {
    // Data comes after the size and name.
    uint32_t offset = sizeof(info->mpy_size) + strlen(info->mpy_name) + 1;
    return (uint8_t *)info + padded(offset);
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
static mpy_info_t *mpy_data_find(const char *name) {

    // Start at first script.
    mpy_info_t *next = mpy_first;

    // Iterate through the programs while not found.
    while (strcmp(next->mpy_name, name)) {
        // Exit if we're passed the end.
        if ((uint8_t *)next >= (uint8_t *)mpy_first + mpy_size_total) {
            return NULL;
        }
        // Get reference to next script.
        next = (mpy_info_t *)(mpy_data_get_buf(next) + padded(next->mpy_size));
    }
    return next;
}

// Runs MicroPython with the given program data.
void pbsys_main_application(pbsys_main_program_t *program) {

    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);

    // MicroPython heap starts after program data.
    gc_init(program->data + program->size, &_heap_end);

    mp_init();
    readline_init0();

    // Set program data reference to first script. This is used to run main,
    // and to set the starting point for finding downloaded modules.
    mpy_data_init(program);

    // Get first script from the user data.
    uint8_t *mpy_data = mpy_data_get_buf(mpy_first);
    uint32_t mpy_size = mpy_first->mpy_size;

    // For backwards compatibility, detect old-format single-script data.
    // TODO: This can be removed once all IDE tools are updated.
    if (program->data[0] == 'M') {
        mpy_data = program->data;
        mpy_size = program->size;
    }

    // Execute the user script or the REPL.
    run_user_program(mpy_size, mpy_data, program->run_builtin);

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

static mp_obj_t pb_import_loaded_module(size_t n_args, const mp_obj_t *args) {

    // For backwards compatibility, skip imports for legacy single-script data.
    // TODO: This can be removed once all IDE tools are updated.
    if (((uint8_t *)mpy_first)[0] == 'M') {
        return MP_OBJ_NULL;
    }

    // Try to find the requested module.
    mpy_info_t *info = mpy_data_find(mp_obj_str_get_str(args[0]));
    if (!info) {
        // Not found, so give up.
        return MP_OBJ_NULL;
    }

    // Parse the static script data.
    mp_reader_t reader;
    pb_reader_user_data_t data;
    pb_reader_user_data_new(&reader, &data, mpy_data_get_buf(info), info->mpy_size);

    // Create new module and execute in its own context.
    mp_obj_t module_obj = mp_obj_new_module(mp_obj_str_get_qstr(args[0]));
    mp_module_context_t *context = MP_OBJ_TO_PTR(module_obj);
    mp_compiled_module_t compiled_module = mp_raw_code_load(&reader, context);
    do_execute_raw_code(context, compiled_module.rc, compiled_module.context);

    // Return the module we found.
    return module_obj;
}

// Overrides MicroPython's mp_builtin___import__
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

    // Pybricks extension: Search for modules in downloaded user data.
    module_obj = pb_import_loaded_module(n_args, args);
    if (module_obj != MP_OBJ_NULL) {
        return module_obj;
    }

    // Couldn't find the module, so fail
    #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
    mp_raise_msg(&mp_type_ImportError, MP_ERROR_TEXT("module not found"));
    #else
    mp_raise_msg_varg(&mp_type_ImportError, MP_ERROR_TEXT("no module named '%q'"), module_name_qstr);
    #endif
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
