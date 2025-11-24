// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup UARTDriver Driver: Universal Asynchronous Receiver/Transmitter (UART)
 * @{
 */

#ifndef _PBDRV_UART_H_
#define _PBDRV_UART_H_

#include <stdint.h>
#include <stddef.h>

#include <pbdrv/config.h>
#include <pbio/error.h>
#include <pbio/os.h>
#include <pbio/port.h>

typedef struct _pbdrv_uart_dev_t pbdrv_uart_dev_t;

#if PBDRV_CONFIG_UART

/**
 * Get an instance of the UART driver.
 *
 * @param [in]  id              The ID of the UART device.
 * @param [out] uart_dev        The UART device.
 */
pbio_error_t pbdrv_uart_get_instance(uint8_t id, pbdrv_uart_dev_t **uart_dev);

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud);
void pbdrv_uart_stop(pbdrv_uart_dev_t *uart_dev);
void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev);

/**
 * Gets the number of bytes in the incoming buffer.
 *
 * @param uart_dev The UART device
 * @return The number of bytes that can be read, or 0 if no data is available
 */
uint32_t pbdrv_uart_in_waiting(pbdrv_uart_dev_t *uart_dev);

/**
 * Asynchronously read from the UART.
 *
 * @param [in]  state     The protothread state.
 * @param [in]  uart_dev  The UART device.
 * @param [out] msg       The buffer to store the received message.
 * @param [in]  length    The length of the expected message.
 * @param [in]  timeout   The timeout in milliseconds or 0 for no timeout.
 * @return The error code.
 */
pbio_error_t pbdrv_uart_read(pbio_os_state_t *state, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint32_t length, uint32_t timeout);

/**
 * Asynchronously write to the UART.
 *
 * @param [in]  state     The protothread state.
 * @param [in]  uart_dev  The UART device.
 * @param [in]  msg       The message to send.
 * @param [in]  length    The length of the message.
 * @param [in]  timeout   The timeout in milliseconds or 0 for no timeout.
 *
 * @return The error code.
 */
pbio_error_t pbdrv_uart_write(pbio_os_state_t *state, pbdrv_uart_dev_t *uart, const uint8_t *msg, uint32_t length, uint32_t timeout);

#else // PBDRV_CONFIG_UART

static inline pbio_error_t pbdrv_uart_get_instance(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
    *uart_dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
}
static inline void pbdrv_uart_stop(pbdrv_uart_dev_t *uart_dev) {
}
static inline pbio_error_t pbdrv_uart_read_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint32_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_read_end(pbdrv_uart_dev_t *uart_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_write_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint32_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_write_end(pbdrv_uart_dev_t *uart_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbdrv_uart_write_cancel(pbdrv_uart_dev_t *uart_dev) {
}
static inline void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev) {
}

static inline uint32_t pbdrv_uart_in_waiting(pbdrv_uart_dev_t *uart_dev) {
    return 0;
}

static inline pbio_error_t pbdrv_uart_write(pbio_os_state_t *state, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint32_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_uart_read(pbio_os_state_t *state, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint32_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_UART

#endif // _PBDRV_UART_H_

/** @} */
