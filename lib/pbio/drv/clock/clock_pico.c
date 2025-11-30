// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Clock driver using Raspberry Pi Pico SDK

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_PICO

#include <stdint.h>

#include <pbdrv/clock.h>

#include "pico/time.h"
#include "pico/types.h"


void pbdrv_clock_init(void) {
    // Pico SDK takes care of this.
}

uint32_t pbdrv_clock_get_us(void) {
    return to_us_since_boot(get_absolute_time());
}

uint32_t pbdrv_clock_get_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

uint32_t pbdrv_clock_get_100us(void) {
    return to_us_since_boot(get_absolute_time()) / 100;
}

#endif // PBDRV_CONFIG_CLOCK_PICO
