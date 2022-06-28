// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef _PBIO_UARTDEV_H_
#define _PBIO_UARTDEV_H_

#include <stddef.h>
#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>
#include <pbio/iodev.h>

typedef struct {
    uint8_t uart_id;    /**< The ID of a UART device used by this uartdev */
} pbio_uartdev_platform_data_t;

#if PBIO_CONFIG_UARTDEV

pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev);
void pbio_uartdev_ready(uint8_t id);

#if !PBIO_CONFIG_UARTDEV_NUM_DEV
#error Must define PBIO_CONFIG_UARTDEV_NUM_DEV
#endif

extern const pbio_uartdev_platform_data_t pbio_uartdev_platform_data[PBIO_CONFIG_UARTDEV_NUM_DEV];

#else // PBIO_CONFIG_UARTDEV

static inline pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev) {
    *iodev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_uartdev_ready(uint8_t id) {
}

#endif // PBIO_CONFIG_UARTDEV

#endif // _PBIO_UARTDEV_H_
