// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_NXT

#include <contiki.h>

#include <nxt/at91sam7.h>
#include <nxt/systick.h>

static void clock_systick_hook(void) {
    etimer_request_poll();
}

void pbdrv_clock_init(void) {
    // TODO: Split NXT init into pbdrv_clock_init etc
    systick_set_hook(clock_systick_hook);
}

uint32_t pbdrv_clock_get_ms(void) {
    return systick_get_ms();
}

uint32_t pbdrv_clock_get_100us(void) {
    // Revisit: derive from ns counter properly.
    return systick_get_ms() * 10;
}

uint32_t pbdrv_clock_get_us(void) {
    return systick_get_us();
}

#endif // PBDRV_CONFIG_CLOCK_NXT
