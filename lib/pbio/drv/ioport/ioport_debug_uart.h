// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_IOPORT_DEBUG_UART_H_
#define _INTERNAL_PBDRV_IOPORT_DEBUG_UART_H_

#include <contiki.h>

#include <pbio/error.h>

#include <pbdrv/config.h>

#if PBDRV_CONFIG_IOPORT_DEBUG_UART

extern struct pt printf_thread;

/**
 * Prints debug information on the last ioport.
 *
 * Will not print anything if the debug uart is already in use by another
 * debug message, to avoid blocking.
 *
 * Useful outside of a protothread or when timing is more critical while
 * complete information is not.
 *
 * @param [in]  format  Format string.
 * @param [in]  ...     Arguments.
 *
 * @return Error code.
 */
pbio_error_t pbdrv_ioport_debug_uart_printf(const char *format, ...);

PT_THREAD(pbdrv_ioport_debug_uart_printf_thread(struct pt *pt, const char *format, ...));

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
    PT_SPAWN(pt, &printf_thread, pbdrv_ioport_debug_uart_printf_thread(&printf_thread, __VA_ARGS__))

#else // PBDRV_CONFIG_IOPORT_DEBUG_UART

static inline pbio_error_t pbdrv_ioport_debug_uart_printf(const char *format, ...) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#define PBDRV_IOPORT_DEBUG_UART_PT_PRINTF(pt, ...)

#endif // PBDRV_CONFIG_IOPORT_DEBUG_UART

#endif // _INTERNAL_PBDRV_IOPORT_DEBUG_UART_H_
