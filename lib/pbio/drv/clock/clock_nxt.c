// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_CLOCK_NXT

#include <contiki.h>

#include <nxt/at91sam7.h>
#include <nxt/systick.h>

#if CLOCK_CONF_SECOND != 1000
#error Clock must be set to 1 msec ticks
#endif

static void clock_systick_hook(void) {
    etimer_request_poll();
}

clock_time_t clock_time(void) {
    return systick_get_ms();
}

uint32_t clock_usecs(void) {
    return systick_get_us();
}

void clock_init(void) {
    // TODO: Split NXT init into clock_init etc
    systick_set_hook(clock_systick_hook);
}

/* The inner loop takes 4 cycles. The outer 5+SPIN_COUNT*4. */

#define SPIN_TIME 2 /* us */
#define SPIN_COUNT (((CLOCK_FREQUENCY * SPIN_TIME / 1000000) - 5) / 4)

void clock_delay_usec(uint16_t t) {
    #ifdef __THUMBEL__
    __asm volatile ("1: mov r1,%2\n2:\tsub r1,#1\n\tbne 2b\n\tsub %0,#1\n\tbne 1b\n"
        : "=l" (t) : "0" (t), "l" (SPIN_COUNT));
    #else
    #error Must be compiled in thumb mode
    #endif
}

#endif // PBDRV_CONFIG_CLOCK_NXT
