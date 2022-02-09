// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_LINUX

#include <stdint.h>
#include <time.h>
#include <unistd.h>

// The SIGNAL option adds a timer that acts as the 1ms tick on embedded systems.

#if PBDRV_CONFIG_CLOCK_LINUX_SIGNAL

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

#include <contiki.h>

#define NSEC_PER_MSEC       1000000

#define TIMER_SIGNAL        SIGRTMIN
#define TIMER_INTERVAL      (1 * NSEC_PER_MSEC)

static pthread_t main_thread;

static void handle_signal(int sig) {
    // since signals can occur on any thread, we need to ensure
    // that we interrupt the main thread. This is needed, e.g.
    // for MicroPython to ensure that any blocking syscall on the
    // main thread is interrupted and the event poll hook runs.
    if (pthread_self() == main_thread) {
        etimer_request_poll();
    } else {
        pthread_kill(main_thread, TIMER_SIGNAL);
    }
}

void pbdrv_clock_init(void) {
    static timer_t clock_timer;
    int err;

    main_thread = pthread_self();

    // set up 1ms tick using signal

    struct sigaction sa = {
        .sa_handler = handle_signal,
    };

    sigemptyset(&sa.sa_mask);

    err = sigaction(TIMER_SIGNAL, &sa, NULL);

    if (err == -1) {
        perror("sigaction");
    }

    struct sigevent se = {
        .sigev_notify = SIGEV_SIGNAL,
        .sigev_signo = TIMER_SIGNAL,
    };

    err = timer_create(CLOCK_REALTIME, &se, &clock_timer);

    if (err == -1) {
        perror("timer_create");
    }

    struct itimerspec its = {
        .it_interval.tv_sec = 0,
        .it_interval.tv_nsec = TIMER_INTERVAL,
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = TIMER_INTERVAL,
    };

    err = timer_settime(clock_timer, 0, &its, NULL);

    if (err == -1) {
        perror("timer_settime");
    }
}

#else // PBDRV_CONFIG_CLOCK_LINUX_SIGNAL

void pbdrv_clock_init(void) {
}

#endif // PBDRV_CONFIG_CLOCK_LINUX_SIGNAL

uint32_t pbdrv_clock_get_ms(void) {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return time_val.tv_sec * 1000 + time_val.tv_nsec / 1000000;
}

uint32_t pbdrv_clock_get_us(void) {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return time_val.tv_sec * 1000000 + time_val.tv_nsec / 1000;
}

#endif // PBDRV_CONFIG_CLOCK_LINUX
