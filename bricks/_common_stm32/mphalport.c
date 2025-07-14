// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

// Contains the MicroPython HAL for STM32-based Pybricks ports.

#include <stdint.h>

#include <contiki.h>

#include <pbdrv/clock.h>
#include <pbdrv/config.h>
#include <pbio/main.h>
#include <pbsys/bluetooth.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"
#include "py/stream.h"

// using "internal" pbdrv variable
extern volatile uint32_t pbdrv_clock_ticks;

// Core delay function that does an efficient sleep and may switch thread context.
// If IRQs are enabled then we must have the GIL.
void mp_hal_delay_ms(mp_uint_t Delay) {
    if (pbdrv_clock_is_ticking()) {
        // IRQs enabled, so can use systick counter to do the delay
        uint32_t start = pbdrv_clock_ticks;
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        do {
            // This macro will execute the necessary idle behaviour.  It may
            // raise an exception, switch threads or enter sleep mode (waiting for
            // (at least) the SysTick interrupt).
            MICROPY_EVENT_POLL_HOOK
        } while (pbdrv_clock_ticks - start < Delay);
    } else {
        // IRQs disabled, so need to use a busy loop for the delay.
        pbdrv_clock_busy_delay_ms(Delay);
    }
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;

    if ((poll_flags & MP_STREAM_POLL_RD) && pbsys_bluetooth_rx_get_available()) {
        ret |= MP_STREAM_POLL_RD;
    }

    return ret;
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    uint32_t size;
    uint8_t c;

    // wait for rx interrupt
    while (size = 1, pbsys_bluetooth_rx(&c, &size) != PBIO_SUCCESS) {
        MICROPY_EVENT_POLL_HOOK
    }

    return c;
}

// Send string of given length
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    uint32_t remaining = len;

    while (remaining) {
        uint32_t size = remaining;

        pbio_error_t err = pbsys_bluetooth_tx((const uint8_t *)str, &size);
        if (err == PBIO_SUCCESS) {
            str += size;
            remaining -= size;
        }

        MICROPY_EVENT_POLL_HOOK

        if (err != PBIO_ERROR_AGAIN) {
            // Ignoring error for now. This means stdout lost if Bluetooth is
            // disconnected.
            break;
        }
    }

    return len - remaining;
}

void mp_hal_stdout_tx_flush(void) {
    while (!pbsys_bluetooth_tx_is_idle()) {
        MICROPY_EVENT_POLL_HOOK
    }
}
