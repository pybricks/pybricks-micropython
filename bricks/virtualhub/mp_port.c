// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

// MicroPython port-specific implementation hooks

#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>

#include <contiki.h>

#include <pbio/main.h>
#include <pbsys/core.h>
#include <pbsys/program_stop.h>
#include <pbsys/status.h>

#include "py/mphal.h"
#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objexcept.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/runtime.h"
#include "py/stream.h"

#include "pybricks/util_pb/pb_error.h"
#include <pybricks/common.h>

// from micropython/ports/unix/main.c
#define FORCED_EXIT (0x100)

// callback for when stop button is pressed in IDE or on hub
void pbsys_main_stop_program(bool force_stop) {
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

// MICROPY_PORT_INIT_FUNC
void pb_virtualhub_port_init(void) {

    pbio_init(true);

    pbsys_init();

    pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);
    while (pbio_do_one_event()) {
    }

    pb_package_pybricks_init(true);

    // HACK: Motor simulation needs some time after pbio init to register
    // one sample of position data.
    mp_hal_delay_ms(100);
}

// MICROPY_PORT_DEINIT_FUNC
void pb_virtualhub_port_deinit(void) {

    pb_package_pybricks_deinit();
}

// MICROPY_VM_HOOK_LOOP
void pb_virtualhub_poll(void) {
    while (pbio_do_one_event()) {
    }
}

// MICROPY_EVENT_POLL_HOOK
void pb_virtualhub_event_poll(void) {
start:
    mp_handle_pending(true);

    int events_handled = 0;

    while (pbio_do_one_event()) {
        events_handled++;
    }

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
        .tv_nsec = 100000,
    };

    // "sleep" with "interrupts" enabled
    MP_THREAD_GIL_EXIT();
    pselect(0, NULL, NULL, NULL, &timeout, &origmask);
    MP_THREAD_GIL_ENTER();

    // restore "interrupts"
    pthread_sigmask(SIG_SETMASK, &origmask, NULL);
}


void pb_virtualhub_delay_us(mp_uint_t us) {
    mp_uint_t start = mp_hal_ticks_us();

    while (mp_hal_ticks_us() - start < us) {
        pb_virtualhub_poll();
    }
}

uintptr_t mp_hal_stdio_poll(uintptr_t flags) {
    struct pollfd fds[] = {
        { .fd = STDIN_FILENO, .events = flags & MP_STREAM_POLL_RD ? POLLIN : 0, },
        { .fd = STDOUT_FILENO, .events = flags & MP_STREAM_POLL_WR ? POLLOUT : 0, },
    };
    int ret;

    MP_HAL_RETRY_SYSCALL(ret, poll(fds, MP_ARRAY_SIZE(fds), 0), mp_raise_OSError(err));

    uintptr_t rflags = 0;

    if (ret > 0) {
        if (fds[0].revents & POLLIN) {
            rflags |= MP_STREAM_POLL_RD;
        }
        if (fds[1].revents & POLLOUT) {
            rflags |= MP_STREAM_POLL_WR;
        }
    }

    return rflags;
}
