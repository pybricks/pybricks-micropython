// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_NXT

#include <contiki.h>

#include <base/at91sam7s256.h>
#include <base/drivers/systick.h>

static void clock_systick_hook(void) {
    etimer_request_poll();
}

void pbdrv_clock_init(void) {
    // TODO: Split NXT init into pbdrv_clock_init etc
    nx_systick_install_scheduler(clock_systick_hook);
}

uint32_t pbdrv_clock_get_ms(void) {
    return nx_systick_get_ms();
}

uint32_t pbdrv_clock_get_100us(void) {
    // Revisit: derive from ns counter properly.
    return nx_systick_get_ms() * 10;
}

uint32_t pbdrv_clock_get_us(void) {
    // TODO
    return nx_systick_get_ms() * 1000;
}

#endif // PBDRV_CONFIG_CLOCK_NXT
