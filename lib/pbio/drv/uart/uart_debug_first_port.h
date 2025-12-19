// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_UART_DEBUG_FIRST_PORT_H_
#define _INTERNAL_PBDRV_UART_DEBUG_FIRST_PORT_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_UART_DEBUG_FIRST_PORT

void pbdrv_uart_debug_printf(const char *format, ...);
void pbdrv_uart_debug_vprintf(const char *format, va_list argptr);

bool pbdrv_uart_debug_is_done(void);

void pbdrv_uart_debug_init(void);

// Returns the next character that should be sent to the UART, or -1 if there
// are not pending log messages. Used only by the panic handler to flush unsent
// data before reboot.
int pbdrv_uart_debug_next_char(void);

#else // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT

#define pbdrv_uart_debug_printf(...)
#define pbdrv_uart_debug_vprintf(format, argptr)

#define pbdrv_uart_debug_is_done() (true)

#define pbdrv_uart_debug_init()

static inline int pbdrv_uart_debug_next_char(void) {
    return -1;
}

#endif // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT

// Convenient shorthand for pbdrv_uart_debug_printf.
#define pbdrv_dbg(...) pbdrv_uart_debug_printf(__VA_ARGS__)

#endif // _INTERNAL_PBDRV_UART_DEBUG_FIRST_PORT_H_
