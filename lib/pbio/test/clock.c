// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <contiki.h>

#define TIMER_SIGNAL SIGRTMIN

static void handle_signal(int sig) {
    etimer_request_poll();
}

void clock_init(void) {
    struct sigaction sa;
    struct sigevent se;
    struct itimerspec its;
    timer_t timer;
    int err;

    // set up 1ms tick using signal

    sa.sa_flags = SA_RESTART;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    err = sigaction(TIMER_SIGNAL, &sa, NULL);
    if (err == -1) {
        perror("sigaction");
    }

    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = TIMER_SIGNAL;
    err = timer_create(CLOCK_REALTIME, &se, &timer);
    if (err == -1) {
        perror("timer_create");
    }

    its.it_value.tv_sec = its.it_interval.tv_sec = 0;
    its.it_value.tv_nsec = its.it_interval.tv_nsec = 1000000;
    err = timer_settime(timer, 0, &its, NULL);
    if (err == -1) {
        perror("timer_settime");
    }
}

clock_time_t clock_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

unsigned long clock_usecs() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000L + ts.tv_nsec / 1000L;
}

void clock_wait(clock_time_t t) {
    struct timespec ts, remain;
    ts.tv_sec = clock_to_msec(t) / 1000;
    ts.tv_nsec = clock_to_msec(t) % 1000 * 1000000;
    for (;;) {
        int ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, &remain);
        if (ret == EINTR) {
            ts = remain;
            continue;
        }
        break;
    }
}

void clock_delay_usec(uint16_t duration) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = duration * 1000;
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
}
