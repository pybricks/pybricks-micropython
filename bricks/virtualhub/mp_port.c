// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// MicroPython port-specific implementation hooks

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>

#include <contiki.h>
#include <Python.h>

#include <pbio/main.h>
#include <pbsys/core.h>
#include <pbsys/program_stop.h>
#include <pbsys/status.h>

#include "../../lib/pbio/drv/virtual.h"

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objexcept.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"

#include "pybricks/util_pb/pb_error.h"
#include <pybricks/common.h>

// from micropython/ports/unix/main.c
#define FORCED_EXIT (0x100)

// callback for when stop button is pressed in IDE or on hub
void pbsys_main_stop_program(void) {
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

bool pbsys_main_stdin_event(uint8_t c) {
    return false;
}

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
    pbio_error_t err = pbdrv_virtual_platform_start(cpython_exception_handler);

    if (err != PBIO_SUCCESS) {
        fprintf(stderr, "failed to start virtual hub\n");
        exit(1);
    }

    pbio_init();

    pbsys_init();

    pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
    while (pbio_do_one_event()) {
    }

    pb_package_pybricks_init(false);
}

// MICROPY_PORT_DEINIT_FUNC
void pb_virtualhub_port_deinit(void) {
    pbio_error_t err = pbdrv_virtual_platform_stop();

    if (err != PBIO_SUCCESS) {
        exit(120);
    }
}

// MICROPY_VM_HOOK_LOOP
void pb_virtualhub_poll(void) {
    while (pbio_do_one_event()) {
    }

    // it is probably a bit inefficient, but we need to keep calling into the
    // CPython runtime here in case MicroPython is running in a tight loop,
    // e.g. `while True: pass`.
    pb_assert(pbdrv_virtual_platform_poll());
}

// MICROPY_EVENT_POLL_HOOK
void pb_virtualhub_event_poll(void) {
start:
    mp_handle_pending(true);

    int events_handled = 0;

    while (pbio_do_one_event()) {
        events_handled++;
    }

    pb_assert(pbdrv_virtual_platform_poll());

    // If there were any pbio events handled, don't sleep because there may
    // be something waiting on one of the events that was just handled.
    if (events_handled) {
        return;
    }

    sigset_t sigmask;
    sigfillset(&sigmask);

    // disable "interrupts"
    sigset_t origmask;
    pthread_sigmask(SIG_SETMASK, &sigmask, &origmask);

    if (process_nevents()) {
        // something was scheduled since the event loop above
        pthread_sigmask(SIG_SETMASK, &origmask, NULL);
        goto start;
    }

    struct timespec timeout = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };

    // "sleep" with "interrupts" enabled
    MP_THREAD_GIL_EXIT();
    pselect(0, NULL, NULL, NULL, &timeout, &origmask);
    MP_THREAD_GIL_ENTER();

    // restore "interrupts"
    pthread_sigmask(SIG_SETMASK, &origmask, NULL);
}

uint64_t pb_virtualhub_time_ns(void) {
    uint64_t value;
    pb_assert(pbdrv_virtual_get_u64("clock", -1, "nanoseconds", &value));
    return value;
}

mp_uint_t pb_virtualhub_ticks_us(void) {
    return pb_virtualhub_time_ns() / 1000;
}

mp_uint_t pb_virtualhub_ticks_ms(void) {
    return pb_virtualhub_time_ns() / 1000000;
}

void pb_virtualhub_delay_us(mp_uint_t us) {
    mp_uint_t start = pb_virtualhub_ticks_us();

    while (pb_virtualhub_ticks_us() - start < us) {
        pb_virtualhub_poll();
    }
}
