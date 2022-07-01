// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

// Clock implementation for tests. This allows tests to exactly control the
// clock ticks to get repeatable tests rather than relying on a system clock.

#include <stdint.h>

#include <contiki.h>

static uint32_t clock_ticks;

/**
 * Increase the current clock ticks and poll etimers.
 * @param [in]  ticks   The number of ticks to add to the clock.
 */
void pbio_test_clock_tick(uint32_t ticks) {
    clock_ticks += ticks;
    etimer_request_poll();
}

void pbdrv_clock_init(void) {
}

uint32_t pbdrv_clock_get_ms(void) {
    return clock_ticks;
}

uint32_t pbdrv_clock_get_100us(void) {
    return clock_ticks * 10;
}

uint32_t pbdrv_clock_get_us(void) {
    return clock_ticks * 1000;
}
