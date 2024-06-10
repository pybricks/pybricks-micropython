// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_TIAM1808

#include <stdint.h>

void pbdrv_clock_init(void) {
    // Actual low level clocks should probably be configured at the very
    // start of in SystemInit in platform.c instead.
    // But optionally, additional things can be initialized here.
}

uint32_t pbdrv_clock_get_us(void) {
    // TODO: TIAM1808 implementation.
    return 0;
}

uint32_t pbdrv_clock_get_ms(void) {
    // TODO: TIAM1808 implementation.
    return 0;
}

uint32_t pbdrv_clock_get_100us(void) {
    // TODO: TIAM1808 implementation (1 count = 100us, so 10 counts per millisecond.)
    return 0;
}

#endif // PBDRV_CONFIG_CLOCK_TIAM1808
