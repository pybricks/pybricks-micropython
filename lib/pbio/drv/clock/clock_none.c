// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_NONE

#include <stdint.h>

void pbdrv_clock_init(void) {
}

uint32_t pbdrv_clock_get_us(void) {
    return 0;
}

uint32_t pbdrv_clock_get_ms(void) {
    return 0;
}

uint32_t pbdrv_clock_get_100us(void) {
    return 0;
}

void pbdrv_clock_busy_delay_ms(uint32_t ms) {
}

bool pbdrv_clock_is_ticking(void) {
    return false;
}

#endif // PBDRV_CONFIG_CLOCK_NONE
