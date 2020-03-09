// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <time.h>
#include <stdint.h>
#include <unistd.h>

#include <contiki.h>

void clock_init(void) {
}

clock_time_t clock_time() {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return time_val.tv_sec * 1000 + time_val.tv_nsec / 1000000;
}

unsigned long clock_usecs() {
    struct timespec time_val;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_val);
    return time_val.tv_sec * 1000000 + time_val.tv_nsec / 1000;
}

void clock_delay_usec(uint16_t duration) {
    // FIXME: is there a way to busy-wait on Linux? maybe call clock_gettime() in a loop?
    usleep(duration);
}
