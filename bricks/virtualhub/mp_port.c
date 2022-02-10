// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// MicroPython port-specific implementation hooks

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <Python.h>

#include <pbio/main.h>
#include <pbsys/user_program.h>

#include "py/mpconfig.h"
#include "py/objtuple.h"
#include "py/obj.h"
#include "py/runtime.h"

// from micropython/ports/unix/main.c
#define FORCED_EXIT (0x100)

#define CREATE_VIRTUAL_HUB \
    "import importlib, os\n" \
    "virtualhub_module = os.environ.get('VIRTUALHUB_MODULE', 'virtualhub')\n" \
    "virtualhub = importlib.import_module(virtualhub_module)\n" \
    "hub = virtualhub.VirtualHub()\n"

static PyThreadState *thread_state;

// callback for when stop button is pressed in IDE or on hub
static void user_program_stop_func(void) {
    static const mp_obj_tuple_t args = {
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

// MICROPY_PORT_INIT_FUNC
void pb_virtualhub_port_init(void) {
    // embedding Python to provide virtual hardware implementation
    Py_Initialize();

    if (PyRun_SimpleString(CREATE_VIRTUAL_HUB) < 0) {
        fprintf(stderr, "failed to create virtualhub\n");
        exit(1);
    }

    // release the GIL to allow MicroPython to run
    thread_state = PyEval_SaveThread();

    pbio_init();

    pbsys_user_program_prepare(&user_program_callbacks);
}

// MICROPY_PORT_DEINIT_FUNC
void pb_virtualhub_port_deinit(void) {
    pbsys_user_program_unprepare();

    PyEval_RestoreThread(thread_state);

    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
}

// MICROPY_EVENT_POLL_HOOK
void pb_virtualhub_event_poll(void) {
    mp_handle_pending(true);

    while (pbio_do_one_event()) {
    }

    PyGILState_STATE state = PyGILState_Ensure();

    PyRun_SimpleString("hub.on_event_poll()\n");

    PyGILState_Release(state);
}
