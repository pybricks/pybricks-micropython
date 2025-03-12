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
#include <contiki.h>

typedef struct _pbdrv_uart_dev_t pbdrv_uart_dev_t;

typedef void (*pbdrv_uart_poll_callback_t)(void *context);

#if PBDRV_CONFIG_UART

/**
 * Get an instance of the UART driver.
 *
 * @param [in]  id              The ID of the UART device.
 * @param [in]  parent_process  The parent process. Allows UART to poll process on IRQ events.
 * @param [out] uart_dev        The UART device.
 */
pbio_error_t pbdrv_uart_get_instance(uint8_t id, struct process *parent_process, pbdrv_uart_dev_t **uart_dev);

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud);
void pbdrv_uart_stop(pbdrv_uart_dev_t *uart_dev);
void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev);

int32_t pbdrv_uart_get_char(pbdrv_uart_dev_t *uart_dev);

/**
 * Asynchronously read from the UART.
 *
 * @param [in]  pt        The protothread.
 * @param [in]  uart_dev  The UART device.
 * @param [out] msg       The buffer to store the received message.
 * @param [in]  length    The length of the expected message.
 * @param [in]  timeout   The timeout in milliseconds or 0 for no timeout.
 * @param [out] err       The error code.
 */
PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err));

/**
 * Asynchronously write to the UART.
 *
 * @param [in]  pt        The protothread.
 * @param [in]  uart_dev  The UART device.
 * @param [in]  msg       The message to send.
 * @param [in]  length    The length of the message.
 * @param [in]  timeout   The timeout in milliseconds or 0 for no timeout.
 * @param [out] err       The error code.
 */
PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err));

#else // PBDRV_CONFIG_UART

static inline pbio_error_t pbdrv_uart_get_instance(uint8_t id, struct process *parent_process, pbdrv_uart_dev_t **uart_dev) {
    *uart_dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud) {
}
static inline void pbdrv_uart_stop(pbdrv_uart_dev_t *uart_dev) {
}
static inline pbio_error_t pbdrv_uart_read_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_read_end(pbdrv_uart_dev_t *uart_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_write_begin(pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_uart_write_end(pbdrv_uart_dev_t *uart_dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline void pbdrv_uart_write_cancel(pbdrv_uart_dev_t *uart_dev) {
}
static inline void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev) {
}

/**
 * Get a character from the UART.
 *
 * @param uart_dev The UART device
 * @return The character read, or -1 if no character is available
 */
static inline int32_t pbdrv_uart_get_char(pbdrv_uart_dev_t *uart_dev) {
    return -1;
}

static inline PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {
    PT_BEGIN(pt);
    *err = PBIO_ERROR_NOT_SUPPORTED;
    PT_END(pt);
}

static inline PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err)) {
    PT_BEGIN(pt);
    *err = PBIO_ERROR_NOT_SUPPORTED;
    PT_END(pt);
}

#endif // PBDRV_CONFIG_UART

#endif // _PBDRV_UART_H_

/** @} */
