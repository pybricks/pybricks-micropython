// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George

#include <unistd.h>
#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 */

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
} periph_uart_t;
#define USART1 ((periph_uart_t*)0x40011000)

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    // wait for RXNE
    while ((USART1->SR & (1 << 5)) == 0) {
    }
    c = USART1->DR;
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        // wait for TXE
        while ((USART1->SR & (1 << 7)) == 0) {
        }
        USART1->DR = *str++;
    }
}
