// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Clock implementation for tests. This allows tests to exactly control the
// clock ticks to get repeatable tests rather than relying on a system clock.

#include <contiki.h>

static clock_time_t clock_ticks;

/**
 * Increase the current clock ticks and poll etimers.
 * @param [in]  ticks   The number of ticks to add to the clock.
 */
void clock_tick(clock_time_t ticks) {
    clock_ticks += ticks;
    etimer_request_poll();
}

void clock_init(void) {
}

clock_time_t clock_time() {
    return clock_ticks;
}

unsigned long clock_usecs() {
    return clock_to_msec(clock_ticks) * 1000UL;
}
