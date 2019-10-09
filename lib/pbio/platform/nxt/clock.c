// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George

#include <pbdrv/config.h>

#include "sys/clock.h"
#include "sys/etimer.h"

#include <nxt/systick.h>

clock_time_t clock_time() {
    return systick_get_ms();
}

uint32_t clock_usecs(void) {
    return systick_get_us();
}

void clock_init(void) {
    // TODO: Split NXT init into clock_init etc
}
