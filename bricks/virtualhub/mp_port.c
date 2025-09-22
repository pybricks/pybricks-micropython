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

#include "pbio_os_config.h"

#include <pbdrv/core.h>

#include <pbio/main.h>
#include <pbio/os.h>
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

    pbdrv_init();
    pbio_init(true);
    pbsys_init();

    pbsys_status_set(PBIO_PYBRICKS_STATUS_USER_PROGRAM_RUNNING);

    while (pbio_os_run_processes_once()) {
    }

    pb_package_pybricks_init(true);
}

// MICROPY_PORT_DEINIT_FUNC
void pb_virtualhub_port_deinit(void) {
}

// Implementation for MICROPY_EVENT_POLL_HOOK
void pb_event_poll_hook(void) {

    while (pbio_os_run_processes_once()) {
    }

    mp_handle_pending(true);

    pbio_os_run_processes_and_wait_for_event();
}

pbio_os_irq_flags_t pbio_os_hook_disable_irq(void) {
    sigset_t sigmask;
    sigfillset(&sigmask);

    sigset_t origmask;
    pthread_sigmask(SIG_SETMASK, &sigmask, &origmask);
    return origmask;
}

void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags) {
    sigset_t origmask = (sigset_t)flags;
    pthread_sigmask(SIG_SETMASK, &origmask, NULL);
}

void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags) {
    struct timespec timeout = {
        .tv_sec = 0,
        .tv_nsec = 100000,
    };
    // "sleep" with "interrupts" enabled
    sigset_t origmask = flags;
    MP_THREAD_GIL_EXIT();
    pselect(0, NULL, NULL, NULL, &timeout, &origmask);
    MP_THREAD_GIL_ENTER();
}

void pb_virtualhub_delay_us(mp_uint_t us) {
    mp_uint_t start = mp_hal_ticks_us();

    while (mp_hal_ticks_us() - start < us) {
        MICROPY_VM_HOOK_LOOP;
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
