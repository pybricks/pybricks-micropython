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

typedef struct {

} pbdrv_uart_dev_t;

typedef void (*pbdrv_uart_poll_callback_t)(void *context);

#if PBDRV_CONFIG_UART

pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev);

void pbdrv_uart_set_baud_rate(pbdrv_uart_dev_t *uart_dev, uint32_t baud);
void pbdrv_uart_stop(pbdrv_uart_dev_t *uart_dev);
void pbdrv_uart_flush(pbdrv_uart_dev_t *uart_dev);
void pbdrv_uart_set_poll_callback(pbdrv_uart_dev_t *uart_dev, pbdrv_uart_poll_callback_t callback, void *context);

int32_t pbdrv_uart_get_char(pbdrv_uart_dev_t *uart_dev);
PT_THREAD(pbdrv_uart_read(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err));
PT_THREAD(pbdrv_uart_write(struct pt *pt, pbdrv_uart_dev_t *uart_dev, uint8_t *msg, uint8_t length, uint32_t timeout, pbio_error_t *err));

#else // PBDRV_CONFIG_UART

static inline pbio_error_t pbdrv_uart_get(uint8_t id, pbdrv_uart_dev_t **uart_dev) {
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
static inline void pbdrv_uart_set_poll_callback(pbdrv_uart_dev_t *uart_dev, pbdrv_uart_poll_callback_t callback, void *context) {
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
