// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#ifndef _PBDRV_COUNTER_COUNTER_H_
#define _PBDRV_COUNTER_COUNTER_H_

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/counter.h>
#include <pbio/error.h>

struct _pbdrv_counter_dev_t {
    pbio_error_t (*get_count)(pbdrv_counter_dev_t *dev, int32_t *count);
    pbio_error_t (*get_rate)(pbdrv_counter_dev_t *dev, int32_t *rate);
    bool initalized;
};

typedef struct {
    pbio_error_t (*init)();
    pbio_error_t (*exit)();
} pbdrv_counter_drv_t;

#if PBDRV_CONFIG_COUNTER
pbio_error_t pbdrv_counter_register(uint8_t id, pbdrv_counter_dev_t *dev);
pbio_error_t pbdrv_counter_unregister(pbdrv_counter_dev_t *dev);
#else // PBDRV_CONFIG_COUNTER
static inline pbio_error_t pbdrv_counter_register(uint8_t id, pbdrv_counter_dev_t *dev) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbdrv_counter_unregister(pbdrv_counter_dev_t *dev) { return PBIO_ERROR_NOT_SUPPORTED; }
#endif // PBDRV_CONFIG_COUNTER

#endif // _PBDRV_COUNTER_COUNTER_H_
