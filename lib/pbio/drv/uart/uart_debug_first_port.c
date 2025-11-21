// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2025 The Pybricks Authors

#include "pbdrv/config.h"

#if PBDRV_CONFIG_UART_DEBUG_FIRST_PORT

#include <pbdrv/uart.h>

#include <pbio/os.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <lwrb/lwrb.h>

#define BUF_SIZE (1024)

static uint8_t ring_buf_storage[BUF_SIZE];
static lwrb_t ring_buffer;

static pbdrv_uart_dev_t *debug_uart = NULL;

/**
 * Formats and stores a string in the UART debug ring buffer.
 *
 * This function works similarly to vprintf, but instead of printing to the
 * standard output. The formatted string will be written to the UART when the
 * buffer is processed.
 *
 * @param format The format string, similar to printf.
 * @param va_list The variable arguments, as a va_list.
 */
void pbdrv_uart_debug_vprintf(const char *format, va_list args) {
    if (!lwrb_is_ready(&ring_buffer)) {
        lwrb_init(&ring_buffer, ring_buf_storage, sizeof(ring_buf_storage));
    }

    char buf[256];
    size_t len = vsnprintf(buf, sizeof(buf), format, args);
    lwrb_write(&ring_buffer, (const uint8_t *)buf, len);

    if (!debug_uart) {
        // Not initialized yet, so just buffer for now.
        return;
    }

    // Request print process to write out new data.
    pbio_os_request_poll();
}

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
    va_list args;
    va_start(args, format);
    pbdrv_uart_debug_vprintf(format, args);
    va_end(args);
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
    static uint8_t *write_buf;
    static size_t write_size;

    PBIO_OS_ASYNC_BEGIN(state);

    if (!lwrb_is_ready(&ring_buffer)) {
        lwrb_init(&ring_buffer, ring_buf_storage, sizeof(ring_buf_storage));
    }

    while (pbdrv_uart_get_instance(0, &debug_uart) != PBIO_SUCCESS) {
        pbio_os_request_poll();
        PBIO_OS_AWAIT_ONCE(state);
    }

    pbdrv_uart_set_baud_rate(debug_uart, 115200);

    for (;;) {
        PBIO_OS_AWAIT_UNTIL(state, lwrb_get_full(&ring_buffer) > 0);

        // Write up to the end of the buffer without wrapping.

        write_buf = lwrb_get_linear_block_read_address(&ring_buffer);
        write_size = lwrb_get_linear_block_read_length(&ring_buffer);
        if (write_size > 256) {
            // Avoid writing too much at once, because it can cause timeouts.
            write_size = 256;
        }
        PBIO_OS_AWAIT(state, &child, (err = pbdrv_uart_write(&child, debug_uart, write_buf, write_size, 100)));
        lwrb_skip(&ring_buffer, write_size);

        if (err != PBIO_SUCCESS) {
            // In most circumstances, we'll have cleared enough to write this
            // error down, unless we're near the end of the ring buffer. If so,
            // then we'll just have to be sad.
            char errbuf[128];
            snprintf(errbuf, sizeof(errbuf), "UART debug write error %d\r\n", err);
            lwrb_write(&ring_buffer, (const uint8_t *)errbuf, strlen(errbuf));
        }
    }

    // Unreachable.
    PBIO_OS_ASYNC_END(PBIO_ERROR_FAILED);
}

/**
 * Checks if the UART debug is done writing.
 */
bool pbdrv_uart_debug_is_done(void) {
    return debug_uart != NULL && lwrb_get_full(&ring_buffer) == 0;
}

void pbdrv_uart_debug_init(void) {
    pbio_os_process_start(&pbdrv_uart_debug_process, pbdrv_uart_debug_process_thread, NULL);
}

#endif // PBDRV_CONFIG_UART_DEBUG_FIRST_PORT
