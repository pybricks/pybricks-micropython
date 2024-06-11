// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_TIAM1808

#include <stdint.h>

#include <contiki.h>

#include <arm920t.h>
#include <systick.h>

/* High priority handler, called 1000 times a second */
static int systick_isr(int ticks) {
    etimer_request_poll();
    return 0;
}

void pbdrv_clock_init(void) {
    // Actual low level clocks should probably be configured at the very
    // start of in SystemInit in platform.c instead.
    // But optionally, additional things can be initialized here.
    systick_set_handler(systick_isr);
    arm_intr_enable();
}

uint32_t pbdrv_clock_get_us(void) {
    // TODO: TIAM1808 implementation.
    return 0;
}

uint32_t pbdrv_clock_get_ms(void) {
    return systick_elapsed();
}

uint32_t pbdrv_clock_get_100us(void) {
    // TODO: TIAM1808 implementation (1 count = 100us, so 10 counts per millisecond.)
    return 0;
}

#endif // PBDRV_CONFIG_CLOCK_TIAM1808
