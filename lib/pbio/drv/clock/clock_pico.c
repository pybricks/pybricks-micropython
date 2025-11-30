// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Clock driver using Raspberry Pi Pico SDK

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_PICO

#include <stdint.h>

#include <pbdrv/clock.h>
#include <pbio/os.h>

#include "hardware/timer.h"
#include "pico/time.h"

static bool pbdrv_clock_pico_timer_callback(repeating_timer_t *rt) {
    pbio_os_request_poll();
    return true;
}

void pbdrv_clock_init(void) {
    static repeating_timer_t timer;
    add_repeating_timer_ms(1, pbdrv_clock_pico_timer_callback, NULL, &timer);
}

uint32_t pbdrv_clock_get_us(void) {
    return time_us_32();
}

uint32_t pbdrv_clock_get_ms(void) {
    return us_to_ms(time_us_64());
}

uint32_t pbdrv_clock_get_100us(void) {
    return time_us_64() / 100;
}

#endif // PBDRV_CONFIG_CLOCK_PICO
