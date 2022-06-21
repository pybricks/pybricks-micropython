// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Virtual counter driver

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_VIRTUAL

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <Python.h>

#include <pbio/util.h>
#include "../virtual.h"
#include "counter.h"

#define DEBUG 0
#if DEBUG
#define dbg_err(s) perror(s)
#else
#define dbg_err(s)
#endif

typedef struct {
    pbdrv_counter_dev_t *dev;
    uint8_t index;
} private_data_t;

static private_data_t private_data[PBDRV_CONFIG_COUNTER_VIRTUAL_NUM_DEV];

static pbio_error_t pbdrv_counter_virtual_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {
    private_data_t *priv = dev->priv;

    pbio_error_t err = pbdrv_virtual_get_i32("counter", priv->index, "rotations", rotations);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return pbdrv_virtual_get_i32("counter", priv->index, "millidegrees", millidegrees);
}

static pbio_error_t pbdrv_counter_virtual_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {
    private_data_t *priv = dev->priv;

    return pbdrv_virtual_get_i32("counter", priv->index, "millidegrees_abs", millidegrees);
}

static const pbdrv_counter_funcs_t pbdrv_counter_virtual_funcs = {
    .get_angle = pbdrv_counter_virtual_get_angle,
    .get_abs_angle = pbdrv_counter_virtual_get_abs_angle,
};

void pbdrv_counter_virtual_init(pbdrv_counter_dev_t *devs) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *priv = &private_data[i];

        priv->index = i;

        // FIXME: assuming that these are the only counter devices
        // counter_id should be passed from platform data instead
        // i.e. enumerate CPython `platform.counter.keys()`.
        _Static_assert(PBDRV_CONFIG_COUNTER_VIRTUAL_NUM_DEV == PBDRV_CONFIG_COUNTER_NUM_DEV,
            "need to fix counter_virtual implementation to allow other counter devices");

        priv->dev = &devs[i];
        priv->dev->funcs = &pbdrv_counter_virtual_funcs;
        priv->dev->priv = priv;
    }
}

#endif // PBDRV_CONFIG_COUNTER_VIRTUAL
