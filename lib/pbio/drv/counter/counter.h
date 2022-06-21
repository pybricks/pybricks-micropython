// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#ifndef _INTERNAL_PBDRV_COUNTER_H_
#define _INTERNAL_PBDRV_COUNTER_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/counter.h>
#include <pbio/error.h>

typedef struct {
    pbio_error_t (*get_angle)(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees);
    pbio_error_t (*get_abs_angle)(pbdrv_counter_dev_t *dev, int32_t *millidegrees);
} pbdrv_counter_funcs_t;

struct _pbdrv_counter_dev_t {
    /** Platform-specific data. */
    const void *pdata;
    /** Driver-specific callback functions. */
    const pbdrv_counter_funcs_t *funcs;
    /** Private instance-specific state. */
    void *priv;
};

#if PBDRV_CONFIG_COUNTER

void pbdrv_counter_init(void);

#else // PBDRV_CONFIG_COUNTER

#define pbdrv_counter_init()

#endif // PBDRV_CONFIG_COUNTER

#endif // _INTERNAL_PBDRV_COUNTER_H_
