// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#ifndef _PBIO_UARTDEV_H_
#define _PBIO_UARTDEV_H_

#include <stddef.h>
#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/iodev.h>

#if PBIO_CONFIG_UARTDEV

pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev);

#if !PBIO_CONFIG_UARTDEV_NUM_DEV
#error Must define PBIO_CONFIG_UARTDEV_NUM_DEV
#endif

typedef struct {
    uint8_t uart_id;    /**< The ID of a UART device used by this uartdev */
    uint8_t counter_id; /**< The ID of a counter device provided by this uartdev */
} pbio_uartdev_platform_data_t;

extern const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV];

#else // PBIO_CONFIG_UARTDEV

static inline pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev) {
    *iodev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_UARTDEV

#endif // _PBIO_UARTDEV_H_
