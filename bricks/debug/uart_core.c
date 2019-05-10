// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George

#include <unistd.h>
#include "py/mpconfig.h"

#include "stm32f446xx.h"

/*
 * Core UART functions to implement for a port
 */

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    // wait for RXNE
    while ((USART6->SR & USART_SR_RXNE) == 0) {
    }
    c = USART6->DR;
    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    while (len--) {
        // wait for TXE
        while ((USART6->SR & USART_SR_TXE) == 0) {
        }
        USART6->DR = *str++;
    }
}
