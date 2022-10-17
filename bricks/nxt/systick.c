// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>

#include <pbdrv/config.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"

#include <nxt/at91sam7.h>
#include <nxt/interrupts.h>
#include <nxt/systick.h>

// Core delay function that does an efficient sleep and may switch thread context.
// If IRQs are enabled then we must have the GIL.
void mp_hal_delay_ms(mp_uint_t Delay) {
    if (interrupts_get()) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = systick_get_ms();
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        do {
            // This macro will execute the necessary idle behaviour.  It may
            // raise an exception, switch threads or enter sleep mode (waiting for
            // (at least) the SysTick interrupt).
            MICROPY_EVENT_POLL_HOOK
        } while (systick_get_ms() - start < Delay);
    } else {
        // IRQs disabled, so need to use a busy loop for the delay.
        systick_wait_ms(Delay);
    }
}

/* The inner loop takes 4 cycles. The outer 5+SPIN_COUNT*4. */

#define SPIN_TIME 2 /* us */
#define SPIN_COUNT (((CLOCK_FREQUENCY * SPIN_TIME / 1000000) - 5) / 4)

void mp_hal_delay_us(uint32_t t) {
    #ifdef __THUMBEL__
    __asm volatile ("1: mov r1,%2\n2:\tsub r1,#1\n\tbne 2b\n\tsub %0,#1\n\tbne 1b\n"
        : "=l" (t) : "0" (t), "l" (SPIN_COUNT));
    #else
    #error Must be compiled in thumb mode
    #endif
}
