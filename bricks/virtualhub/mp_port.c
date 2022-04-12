// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// MicroPython port-specific implementation hooks

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <Python.h>

#include <pbio/main.h>
#include <pbsys/user_program.h>
#include "../../lib/pbio/drv/virtual.h"

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objexcept.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"

#include "pybricks/util_pb/pb_error.h"

// from micropython/ports/unix/main.c
#define FORCED_EXIT (0x100)

// callback for when stop button is pressed in IDE or on hub
static void user_program_stop_func(void) {
    static const mp_rom_obj_tuple_t args = {
        .base = { .type = &mp_type_tuple },
        .len = 2,
        .items = {
            // NB: currently, first arg has to be FORCED_EXIT for default
            // unix unhandled exception handler.
            // https://github.com/micropython/micropython/pull/8151
            MP_ROM_INT(FORCED_EXIT),
            MP_ROM_QSTR(MP_QSTR_stop_space_button_space_pressed),
        },
    };

    static mp_obj_exception_t system_exit;

    // Schedule SystemExit exception.
    system_exit.base.type = &mp_type_SystemExit;
    system_exit.traceback_alloc = 0;
    system_exit.traceback_len = 0;
    system_exit.traceback_data = NULL;
    system_exit.args = (mp_obj_tuple_t *)&args;

    mp_sched_exception(MP_OBJ_FROM_PTR(&system_exit));
}

static const pbsys_user_program_callbacks_t user_program_callbacks = {
    .stop = user_program_stop_func,
    .stdin_event = NULL,
};

static MP_DEFINE_EXCEPTION(CPythonException, Exception)

static bool cpython_exception_handler(PyObject *type, PyObject *value, PyObject *traceback) {
    // special case for CPython SystemExit - raise SystemExit in MicroPython
    if (PyObject_IsSubclass(type, PyExc_SystemExit) == 1) {
        static const MP_DEFINE_STR_OBJ(message, "SystemExit from CPython");

        static const mp_rom_obj_tuple_t args = {
            .base = { .type = &mp_type_tuple },
            .len = 2,
            .items = {
                // NB: currently, first arg has to be FORCED_EXIT for default
                // unix unhandled exception handler.
                // https://github.com/micropython/micropython/pull/8151
                MP_ROM_INT(FORCED_EXIT),
                MP_ROM_PTR(&message),
            },
        };

        static mp_obj_exception_t system_exit;

        // Schedule SystemExit exception.
        system_exit.base.type = &mp_type_SystemExit;
        system_exit.traceback_alloc = 0;
        system_exit.traceback_len = 0;
        system_exit.traceback_data = NULL;
        system_exit.args = (mp_obj_tuple_t *)&args;

        mp_sched_exception(MP_OBJ_FROM_PTR(&system_exit));

        return true;
    }

    // special case for CPython KeyboardInterrupt - raise KeyboardInterrupt in MicroPython
    if (PyObject_IsSubclass(type, PyExc_KeyboardInterrupt) == 1) {
        mp_sched_keyboard_interrupt();
        return true;
    }

    mp_sched_exception(mp_obj_new_exception(&mp_type_CPythonException));

    return false;
}

// MICROPY_PORT_INIT_FUNC
void pb_virtualhub_port_init(void) {
    pbio_error_t err = pbdrv_virtual_start(cpython_exception_handler);

    if (err != PBIO_SUCCESS) {
        fprintf(stderr, "failed to start virtual hub\n");
        exit(1);
    }

    pbio_init();

    pbsys_user_program_prepare(&user_program_callbacks);
}

// MICROPY_PORT_DEINIT_FUNC
void pb_virtualhub_port_deinit(void) {
    pbsys_user_program_unprepare();

    pbio_error_t err = pbdrv_virtual_stop();

    if (err != PBIO_SUCCESS) {
        exit(120);
    }
}

// MICROPY_EVENT_POLL_HOOK
void pb_virtualhub_event_poll(void) {
    mp_handle_pending(true);

    while (pbio_do_one_event()) {
    }

    pb_assert(pbdrv_virtual_poll_events());
}
