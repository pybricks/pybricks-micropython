// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include <signal.h>
#include <sys/select.h>

#include <pbio/os.h>

#include "py/runtime.h"
#include "py/mphal.h"

#if PBDRV_CONFIG_CLOCK_TEST
#include <pbdrv/../../drv/clock/clock_test.h>
#endif

pbio_os_irq_flags_t pbio_os_hook_disable_irq(void) {
    sigset_t sigmask;

    #if PBDRV_CONFIG_CLOCK_TEST
    sigemptyset(&sigmask);
    return sigmask;
    #endif

    sigfillset(&sigmask);
    sigset_t origmask;
    pthread_sigmask(SIG_SETMASK, &sigmask, &origmask);
    return origmask;
}

void pbio_os_hook_enable_irq(pbio_os_irq_flags_t flags) {
    #if PBDRV_CONFIG_CLOCK_TEST
    return;
    #endif

    sigset_t origmask = (sigset_t)flags;
    pthread_sigmask(SIG_SETMASK, &origmask, NULL);
}

void pbio_os_hook_wait_for_interrupt(pbio_os_irq_flags_t flags) {
    #if PBDRV_CONFIG_CLOCK_TEST
    // All events have been handled at this time. Advance the clock
    // and continue immediately.
    pbio_test_clock_tick(1);
    return;
    #endif

    struct timespec timeout = {
        .tv_sec = 0,
        .tv_nsec = 100000,
    };
    // "sleep" with "interrupts" enabled
    sigset_t origmask = flags;
    MP_THREAD_GIL_EXIT();
    pselect(0, NULL, NULL, NULL, &timeout, &origmask);
    MP_THREAD_GIL_ENTER();

    // There is a new timer event to handle.
    pbio_os_request_poll();
}
