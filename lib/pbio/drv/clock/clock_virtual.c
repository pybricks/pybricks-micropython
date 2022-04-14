// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_VIRTUAL

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <contiki.h>

#include <pbio/error.h>

#include "../virtual.h"

#define NSEC_PER_MSEC       1000000
#define NSEC_PER_USEC       1000

#define TIMER_SIGNAL        SIGRTMIN

static void handle_signal(int sig) {
    etimer_request_poll();
}

void pbdrv_clock_init(void) {
    int ret;

    struct sigaction sa = {
        .sa_handler = handle_signal,
    };

    ret = sigemptyset(&sa.sa_mask);

    if (ret == -1) {
        perror("fatal error: sigemptyset failed");
        exit(1);
    }

    ret = sigaction(TIMER_SIGNAL, &sa, NULL);

    if (ret == -1) {
        perror("fatal error: sigaction failed");
        exit(1);
    }

    ssize_t thread_id;
    pbio_error_t err = pbdrv_virtual_get_thread_ident(&thread_id);

    if (err != PBIO_SUCCESS) {
        fprintf(stderr, "fatal error: pbdrv_clock_init failed\n");
        exit(1);
    }

    err = pbdrv_virtual_call_method("clock", -1, "on_init", "ni", thread_id, TIMER_SIGNAL);

    if (err != PBIO_SUCCESS) {
        fprintf(stderr, "fatal error: pbdrv_clock_init failed\n");
        exit(1);
    }
}

uint32_t pbdrv_clock_get_ms(void) {
    uint64_t value;
    pbio_error_t err = pbdrv_virtual_get_u64("clock", -1, "nanoseconds", &value);

    if (err != PBIO_SUCCESS) {
        fprintf(stderr, "fatal error: pbdrv_clock_get_ms failed\n");
        exit(1);
    }

    return value / NSEC_PER_MSEC;
}

uint32_t pbdrv_clock_get_us(void) {
    uint64_t value;
    pbio_error_t err = pbdrv_virtual_get_u64("clock", -1, "nanoseconds", &value);

    if (err != PBIO_SUCCESS) {
        fprintf(stderr, "fatal error: pbdrv_clock_get_us failed\n");
        exit(1);
    }

    return value / NSEC_PER_USEC;
}

#endif // PBDRV_CONFIG_CLOCK_VIRTUAL
