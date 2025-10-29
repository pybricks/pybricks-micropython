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

int32_t pbdrv_uart_debug_get_char(void);

bool pbdrv_uart_debug_is_done(void);

void pbdrv_uart_debug_init(void);

#else // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT

#define pbdrv_uart_debug_printf(...)
#define pbdrv_uart_debug_vprintf(format, argptr)

#define pbdrv_uart_debug_get_char() (-1)

#define pbdrv_uart_debug_is_done() (true)

#define pbdrv_uart_debug_init()

#endif // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT

#endif // _INTERNAL_PBDRV_UART_DEBUG_FIRST_PORT_H_
