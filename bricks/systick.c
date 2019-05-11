// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>

#include <pbdrv/config.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"

// using private pbdrv variable
extern volatile uint32_t clock_time_ticks;

// Core delay function that does an efficient sleep and may switch thread context.
// If IRQs are enabled then we must have the GIL.
void mp_hal_delay_ms(mp_uint_t Delay) {
    if (__get_PRIMASK() == 0) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = clock_time_ticks;
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        while (clock_time_ticks - start < Delay) {
            // This macro will execute the necessary idle behaviour.  It may
            // raise an exception, switch threads or enter sleep mode (waiting for
            // (at least) the SysTick interrupt).
            MICROPY_EVENT_POLL_HOOK
        }
    } else {
        // IRQs disabled, so need to use a busy loop for the delay.
        // To prevent possible overflow of the counter we use a double loop.
        const uint32_t count_1ms = PBDRV_CONFIG_SYS_CLOCK_RATE / 4000;
        for (int i = 0; i < Delay; i++) {
            for (uint32_t count = 0; ++count <= count_1ms;) {
            }
        }
    }
}
