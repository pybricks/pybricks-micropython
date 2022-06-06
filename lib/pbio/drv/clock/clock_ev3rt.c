// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_EV3RT

#include "target_timer.h"

void pbdrv_clock_init(void) {
}

uint32_t pbdrv_clock_get_us(void) {
    // Requires EV3RT kernel domain access.
    // We can't use the user level get_tim
    // call because it locks up when called
    // too frequently.
    return target_hrt_get_current();
}

uint32_t pbdrv_clock_get_ms(void) {
    return pbdrv_clock_get_us() / 1000;
}

uint32_t pbdrv_clock_get_100us(void) {
    return pbdrv_clock_get_us() / 100;
}

#endif // PBDRV_CONFIG_CLOCK_EV3RT
