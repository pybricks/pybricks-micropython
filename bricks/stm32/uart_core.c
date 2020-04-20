// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbsys/sys.h>

#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 */

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    uint8_t c;

    // wait for rx interrupt
    while (pbsys_stdin_get_char(&c) != PBIO_SUCCESS) {
        MICROPY_EVENT_POLL_HOOK
    }

    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    uint8_t c;

    while (len--) {
        c = *str++;
        while (pbsys_stdout_put_char(c) == PBIO_ERROR_AGAIN) {
            // only run pbio events here - don't want keyboard interrupt in middle of printf()
            MICROPY_VM_HOOK_LOOP
        }
    }
}
