// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors
// Copyright (c) 2013, 2014 Damien P. George

// Contains the MicroPython HAL for STM32-based Pybricks ports.

#include <stdint.h>
#include <string.h>

#include <pbdrv/clock.h>
#include <pbdrv/config.h>
#include <pbio/main.h>
#include <pbsys/host.h>

#include "py/ringbuf.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mpconfig.h"
#include "py/stream.h"

#include "hardware/uart.h"
#include "hardware/irq.h"

extern int mp_interrupt_char;
static bool ended_on_new_line = true;

#ifndef MICROPY_HW_STDIN_BUFFER_LEN
#define MICROPY_HW_STDIN_BUFFER_LEN 512
#endif

static uint8_t stdin_ringbuf_array[MICROPY_HW_STDIN_BUFFER_LEN];
ringbuf_t stdin_ringbuf = { stdin_ringbuf_array, sizeof(stdin_ringbuf_array) };

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
        mp_event_wait_indefinite();
    } while (pbdrv_clock_get_ms() - start < Delay);
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    if (poll_flags & MP_STREAM_POLL_WR) {
        ret |= MP_STREAM_POLL_WR;
    }
    return ret;
}

void uart_irq(void) {
    uart_get_hw(uart_default)->icr = UART_UARTICR_BITS; // clear interrupt flags
    if (uart_is_readable(uart_default)) {
        int c = uart_getc(uart_default);
        #if MICROPY_KBD_EXCEPTION
        if (c == mp_interrupt_char) {
            mp_sched_keyboard_interrupt();
            return;
        }
        #endif
        ringbuf_put(&stdin_ringbuf, c);
    }
}

void mp_uart_init(void) {
    uart_get_hw(uart_default)->imsc = UART_UARTIMSC_BITS; // enable mask
    uint irq_num = uart_get_index(uart_default) ? UART1_IRQ : UART0_IRQ;
    irq_set_exclusive_handler(irq_num, uart_irq);
    irq_set_enabled(irq_num, true); // enable irq
}

void mp_uart_write_strn(const char *str, size_t len) {
    uart_write_blocking(uart_default, (const uint8_t *)str, len);
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int c = ringbuf_get(&stdin_ringbuf);
        if (c != -1) {
            return c;
        }
        mp_event_wait_indefinite();
    }
}

// Send string of given length
mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    mp_uart_write_strn(str, len);
    return len;
}


static void pb_stdout_flush(void) {
    // Not implemented.
}

/**
 * Flushes stdout and adds a newline if the last printed character was not a
 * new line.
 */
void pb_stdout_flush_to_new_line(void) {

    pb_stdout_flush();

    // A program may be interrupted in the middle of a long print, or the user
    // may have printed without a newline. Ensure we end on a new line.
    if (!ended_on_new_line) {

        // We have just flushed, so we should be able to easily buffer two more
        // characters. It is only for aesthetics, so it is not critical if this
        // fails. We flush again below just in case.
        const char *eol = "\r\n";
        uint32_t size = strlen(eol);
        mp_hal_stdout_tx_strn(eol, size);
        ended_on_new_line = true;

        pb_stdout_flush();
    }
}
