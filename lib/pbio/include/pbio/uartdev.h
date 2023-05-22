// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

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

/** @cond INTERNAL */
pbio_error_t pbio_uartdev_get(uint8_t id, pbio_iodev_t **iodev);
void pbio_uartdev_ready(uint8_t id);
/** @endcond */

pbio_error_t pbio_uartdev_is_ready(pbio_iodev_t *iodev);
pbio_error_t pbio_uartdev_set_mode(pbio_iodev_t *iodev, uint8_t mode);
pbio_error_t pbio_uartdev_set_mode_with_data(pbio_iodev_t *iodev, uint8_t mode, const void *data);
pbio_error_t pbio_uartdev_get_data(pbio_iodev_t *iodev, uint8_t mode, void **data);

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

static inline pbio_error_t pbio_uartdev_is_ready(pbio_iodev_t *iodev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_uartdev_set_mode(pbio_iodev_t *iodev, uint8_t mode) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_uartdev_set_mode_with_data(pbio_iodev_t *iodev, uint8_t mode, const void *data) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_uartdev_get_data(pbio_iodev_t *iodev, uint8_t mode, void **data) {
    *data = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_UARTDEV

#endif // _PBIO_UARTDEV_H_
