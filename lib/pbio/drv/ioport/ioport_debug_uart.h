// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_DEBUG_UART_H_
#define _INTERNAL_PBDRV_IOPORT_DEBUG_UART_H_

#include <contiki.h>

#include <pbio/error.h>

#include <pbdrv/config.h>
#include <pbdrv/uart.h>

#if PBDRV_CONFIG_IOPORT_DEBUG_UART

extern char debug_buffer[];
extern struct pt debug_printf_thread;
extern pbdrv_uart_dev_t *debug_uart;
extern pbio_error_t debug_err;

PT_THREAD(pbdrv_ioport_debug_uart_debug_printf_thread(struct pt *pt, const char *format, ...));

void pbdrv_ioport_debug_uart_printf_buffer(const char *format, ...);

/**
 * Spawns a task to print debug information on the last ioport.
 *
 * Useful for debugging from a protothread. This will pause the protothread
 * until the debug message is sent, so must not be used when timing is critical.
 *
 * @param [in]  pt     Protothread to spawn on.
 * @param [in]  ...    Format string and arguments.
 */
#define PBDRV_IOPORT_DEBUG_UART_PT_PRINTF(pt, ...) \
    pbdrv_ioport_debug_uart_printf_buffer(__VA_ARGS__); \
    if (debug_uart) { \
        PT_SPAWN(pt, &debug_printf_thread, pbdrv_uart_write(&debug_printf_thread, debug_uart, (uint8_t *)debug_buffer, strlen(debug_buffer), 250, &debug_err)); \
    } \

pbio_error_t pbdrv_ioport_debug_uart_init(pbdrv_uart_poll_callback_t callback);

#else // PBDRV_CONFIG_IOPORT_DEBUG_UART

#define PBDRV_IOPORT_DEBUG_UART_PT_PRINTF(pt, ...)

#endif // PBDRV_CONFIG_IOPORT_DEBUG_UART

#endif // _INTERNAL_PBDRV_IOPORT_DEBUG_UART_H_
