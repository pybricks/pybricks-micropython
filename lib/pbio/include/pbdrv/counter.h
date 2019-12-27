// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

/**
 * \addtogroup CounterDriver Counter driver
 * @{
 */

#ifndef _PBDRV_COUNTER_H_
#define _PBDRV_COUNTER_H_

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

typedef struct _pbdrv_counter_dev_t pbdrv_counter_dev_t;

#if PBDRV_CONFIG_COUNTER

pbio_error_t pbdrv_counter_get(uint8_t id, pbdrv_counter_dev_t **dev);
pbio_error_t pbdrv_counter_get_count(pbdrv_counter_dev_t *dev, int32_t *count);
pbio_error_t pbdrv_counter_get_abs_count(pbdrv_counter_dev_t *dev, int32_t *count);
pbio_error_t pbdrv_counter_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate);

#if !PBDRV_CONFIG_COUNTER_NUM_DEV
#error Must define PBDRV_CONFIG_COUNTER_NUM_DEV
#endif

#else // PBDRV_CONFIG_COUNTER

static inline pbio_error_t pbdrv_counter_get(uint8_t id, pbdrv_counter_dev_t **dev) {
    *dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_counter_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    *count = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_counter_get_abs_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    *count = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbdrv_counter_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    *rate = 0;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_COUNTER

#endif // _PBDRV_COUNTER_H_

/** @} */
