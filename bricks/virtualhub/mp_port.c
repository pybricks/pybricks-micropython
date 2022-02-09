// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// MicroPython port-specific implementation hooks

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <Python.h>

#include <pbio/main.h>

#include "py/runtime.h"

#define CREATE_VIRTUAL_HUB \
    "import importlib, os\n" \
    "virtualhub_module = os.environ.get('VIRTUALHUB_MODULE', 'virtualhub')\n" \
    "virtualhub = importlib.import_module(virtualhub_module)\n" \
    "hub = virtualhub.VirtualHub()\n"

static PyThreadState *thread_state;

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
}

// MICROPY_PORT_DEINIT_FUNC
void pb_virtualhub_port_deinit(void) {
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
