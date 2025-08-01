// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>

#include <contiki.h>

#include <nxos/drivers/systick.h>
#include <nxos/drivers/bt.h>

#include <pbdrv/clock.h>
#include <pbio/error.h>
#include <pbsys/main.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"
#include "py/stream.h"

// Core delay function that does an efficient sleep and may switch thread context.
// We must have the GIL.
void mp_hal_delay_ms(mp_uint_t Delay) {
    // Use systick counter to do the delay
    uint32_t start = pbdrv_clock_get_ms();
    // Wraparound of tick is taken care of by 2's complement arithmetic.
    do {
        // This macro will execute the necessary idle behaviour.  It may
        // raise an exception, switch threads or enter sleep mode (waiting for
        // (at least) the SysTick interrupt).
        MICROPY_EVENT_POLL_HOOK
    } while (pbdrv_clock_get_ms() - start < Delay);
}

// delay for given number of microseconds
void mp_hal_delay_us(uint32_t usec) {
    // TODO
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;

    if ((poll_flags & MP_STREAM_POLL_RD) && nx_bt_stream_opened() && nx_bt_stream_data_read()) {
        ret |= MP_STREAM_POLL_RD;
    }

    return ret;
}

extern bool nx_bt_is_ready(void);

// Receive single character
int mp_hal_stdin_rx_chr(void) {

    while (!nx_bt_is_ready()) {
        MICROPY_EVENT_POLL_HOOK
    }

    uint8_t rx_char;

    // Start reading again for next char
    nx_bt_stream_read(&rx_char, sizeof(rx_char));

    // wait for data to be read
    while (nx_bt_stream_data_read() != sizeof(rx_char)) {
        MICROPY_EVENT_POLL_HOOK
    }

    return rx_char;
}

// Send string of given length
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {

    while (!nx_bt_is_ready()) {
        MICROPY_EVENT_POLL_HOOK
    }

    // Nothing to do if disconnected or empty data
    if (!nx_bt_stream_opened() || len == 0) {
        return len;
    }

    nx_bt_stream_write((uint8_t *)str, len);
    while (!nx_bt_stream_data_written()) {
        MICROPY_EVENT_POLL_HOOK;
    }

    return len;
}

void mp_hal_stdout_tx_flush(void) {
    // currently not buffered
}
