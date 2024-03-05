// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

#include <stdint.h>

#include <contiki.h>

#include <pbdrv/config.h>
#include <pbio/main.h>
#include <pbsys/bluetooth.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"
#include "py/stream.h"

void pb_stack_get_info(char **sstack, char **estack) {

    volatile int stack_dummy;
    // extern uint32_t _estack;
    // extern uint32_t _sstack;
    *sstack = (char *)&stack_dummy;
    *estack = *sstack + 1024 * 64;
}

void pb_event_poll_hook_leave(void) {
}

void mp_hal_delay_ms(mp_uint_t Delay) {
}


uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    return ret;
}

int mp_hal_stdin_rx_chr(void) {
    uint8_t c = 0;
    return c;
}

typedef struct {
    volatile uint8_t *thr;
    volatile uint8_t *lsr;
} pb_hal_uart_t;

// Sensor port 1
static pb_hal_uart_t UART0 = { .thr = (volatile uint8_t *)0x01D0C000, .lsr = (volatile uint8_t *)0x01D0C014 };

// Define the write_str function
static void write_str(pb_hal_uart_t *uart, const char *s) {
    while (*s) {
        while ((*uart->lsr & (1 << 5)) == 0) {
        }
        *uart->thr = *s++;
    }
}

void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    write_str(&UART0, str);
    MICROPY_EVENT_POLL_HOOK
}

void mp_hal_stdout_tx_flush(void) {
}
