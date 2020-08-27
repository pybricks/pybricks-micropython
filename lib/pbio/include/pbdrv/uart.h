// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup UARTDriver Driver: Universal Asynchronous Receiver/Transmitter (UART)
 * @{
 */

#ifndef _PBDRV_UART_H_
#define _PBDRV_UART_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/port.h>

typedef struct {

} pbdrv_uart_dev_t;

#if PBDRV_CONFIG_UART

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev);

/**
 * Sets the baud rate.
 * @param [in]  uart    The UART device
 * @param [in]  baud    The baud rate
 * @return              ::PBIO_SUCCESS if the baud rate was set or
 *                      ::PBIO_ERROR_INVALID_PORT if the *port* does not have a
 *                      UART associated with it.
 */
pbio_error_t pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart, uint32_t baud);

pbio_error_t pbdrv_uart_read_begin(pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout);
pbio_error_t pbdrv_uart_read_end(pbdrv_uart_dev_t *uart);
void pbdrv_uart_read_cancel(pbdrv_uart_dev_t *uart);
pbio_error_t pbdrv_uart_write_begin(pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout);
pbio_error_t pbdrv_uart_write_end(pbdrv_uart_dev_t *uart);
void pbdrv_uart_write_cancel(pbdrv_uart_dev_t *uart);

#else // PBDRV_CONFIG_UART

static inline pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    *uart_dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart, uint32_t baud) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_read_begin(pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_read_end(pbdrv_uart_dev_t *uart) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbdrv_uart_read_cancel(pbdrv_uart_dev_t *uart) {
}
static inline pbio_error_t pbdrv_uart_write_begin(pbdrv_uart_dev_t *uart, uint8_t *msg, uint8_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_write_end(pbdrv_uart_dev_t *uart) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbdrv_uart_write_cancel(pbdrv_uart_dev_t *uart) {
}

#endif // PBDRV_CONFIG_UART

#endif // _PBDRV_UART_H_

/** @} */
