// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors


#include <stdint.h>

typedef struct {
    volatile uint8_t *thr;
    volatile uint8_t *lsr;
} debug_uart_t;

// Sensor port 1
static debug_uart_t UART0 = { .thr = (volatile uint8_t *)0x01D0C000, .lsr = (volatile uint8_t *)0x01D0C014 };

static void debug(debug_uart_t *uart, const char *s) {
    while (*s) {
        while ((*uart->lsr & (1 << 5)) == 0) {
        }
        *uart->thr = *s++;
    }
}

// Called from assembly code in startup.s. After this, the "main" function in
// lib/pbio/sys/main.c is called. That contains all calls to the driver
// initialization (low level in pbdrv, high level in pbio), and system level
// functions for running user code (currently a hardcoded MicroPython script).
void SystemInit(void) {
    debug(&UART0, "System init in platform.c called from startup.s\n\n");
    // TODO: TIAM1808 system init
}
