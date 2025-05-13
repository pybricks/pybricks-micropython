// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2025 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_UART_DEBUG_FIRST_PORT

#include <pbdrv/uart.h>

#include <pbio/os.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define BUF_SIZE (256)

static uint8_t ring_buf[BUF_SIZE];
static size_t ring_head = 0;
static size_t ring_tail = 0;

static pbdrv_uart_dev_t *debug_uart = NULL;

/**
 * Formats and stores a string in the UART debug ring buffer.
 *
 * This function works similarly to printf, but instead of printing to the
 * standard output. The formatted string will be written to the UART when the
 * buffer is processed.
 *
 * @param format The format string, similar to printf.
 * @param ... The variable arguments, similar to printf.
 */
void pbdrv_uart_debug_printf(const char *format, ...) {

    char buf[BUF_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(ring_buf), format, args);
    va_end(args);

    size_t len = strlen(buf);
    for (size_t i = 0; i < len; i++) {
        ring_buf[ring_head] = buf[i];
        ring_head = (ring_head + 1) % BUF_SIZE;
    }

    if (!debug_uart) {
        // Not initialized yet, so just buffer for now.
        return;
    }

    // Request print process to write out new data.
    pbio_os_request_poll();
}

/**
 * Gets a character from the UART debug port.
 *
 * @return The character received or -1 if no character is available.
 */
int32_t pbdrv_uart_debug_get_char(void) {
    if (!debug_uart) {
        return -1;
    }
    return pbdrv_uart_get_char(debug_uart);
}

static pbio_os_process_t pbdrv_uart_debug_process;

static pbio_error_t pbdrv_uart_debug_process_thread(pbio_os_state_t *state, void *context) {

    static pbio_os_state_t child;
    static pbio_error_t err;
    static size_t write_size;

    PBIO_OS_ASYNC_BEGIN(state);

    while (pbdrv_uart_get_instance(0, &debug_uart) != PBIO_SUCCESS) {
        PBIO_OS_AWAIT_ONCE_AND_POLL(state);
    }

    pbdrv_uart_set_baud_rate(debug_uart, 115200);

    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, ring_head != ring_tail);

        // Write up to the end of the buffer without wrapping.
        size_t end = ring_head > ring_tail ? ring_head: BUF_SIZE;
        write_size = end - ring_tail;
        PBIO_OS_AWAIT(state, &child, pbdrv_uart_write(&child, debug_uart, &ring_buf[ring_tail], write_size, 100));
        ring_tail = (ring_tail + write_size) % BUF_SIZE;

        // Reset on failure.
        if (err != PBIO_SUCCESS) {
            ring_head = 0;
            ring_tail = 0;
            continue;
        }

        // Poll to write again if not fully finished, i.e. when wrapping.
        if (ring_head != ring_tail) {
            pbio_os_request_poll();
        }
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

/**
 * Checks if the UART debug is done writing.
 */
bool pbdrv_uart_debug_is_done(void) {
    return debug_uart != NULL && ring_head == ring_tail;
}

void pbdrv_uart_debug_init(void) {
    pbio_os_process_start(&pbdrv_uart_debug_process, pbdrv_uart_debug_process_thread, NULL);
}

#endif // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT
