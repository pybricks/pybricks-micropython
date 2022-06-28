// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// LPF2 Counter driver
//
// This driver parses type-specific position readouts from LPF2 motors.
//
// This driver currently requires that pbdrv_counter_lpf2_get_count gets
// called often enough to observe full-rotation transitions of motors that
// use absolute encoders.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_LPF2

#include <stdint.h>

#include <pbdrv/ioport.h>
#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/util.h>

#include "counter.h"
#include "counter_lpf2.h"

typedef struct {
    int32_t rotations;
    int32_t last_abs_pos;
} private_data_t;

#define PBDRV_COUNTER_UNKNOWN_ABS_ANGLE (INT32_MAX)

static private_data_t private_data[PBDRV_CONFIG_COUNTER_LPF2_NUM_DEV];

static pbio_error_t pbdrv_counter_lpf2_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {

    private_data_t *priv = dev->priv;
    const pbdrv_counter_lpf2_platform_data_t *pdata = dev->pdata;

    // Get iodev, and test if still connected.
    pbio_iodev_t *iodev;
    pbio_error_t err = pbdrv_ioport_get_iodev(pdata->port_id, &iodev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Ensure we are still dealing with a motor.
    if (!PBIO_IODEV_IS_FEEDBACK_MOTOR(iodev)) {
        return PBIO_ERROR_NO_DEV;
    }

    // Ensure that we are on expected mode.
    uint8_t mode_id = PBIO_IODEV_IS_ABS_MOTOR(iodev) ?
        PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB:
        PBIO_IODEV_MODE_PUP_REL_MOTOR__POS;
    if (iodev->mode != mode_id) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get LPF2 data buffer.
    uint8_t *data;
    err = pbio_iodev_get_data(iodev, &data);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // For incremental encoders, we can just return the LPF2 data directly.
    if (!PBIO_IODEV_IS_ABS_MOTOR(iodev)) {
        *count = pbio_get_uint32_le(data);
        priv->last_abs_pos = PBDRV_COUNTER_UNKNOWN_ABS_ANGLE;
        return PBIO_SUCCESS;
    }

    // For absolute encoders, we need to keep track of whole rotations.
    int32_t abs_pos = (int16_t)pbio_get_uint16_le(data + 2);

    // Update rotation counter as encoder passes through 0
    if (priv->last_abs_pos > 2700 && abs_pos < 900) {
        priv->rotations += 1;
    }
    if (priv->last_abs_pos < 900 && abs_pos > 2700) {
        priv->rotations -= 1;
    }
    priv->last_abs_pos = abs_pos;

    // The total count is the position in this circle plus full rotations.
    *count = priv->rotations * 360 + (abs_pos + 5) / 10;
    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_counter_lpf2_get_abs_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    private_data_t *priv = dev->priv;

    // Get total count value, which also updates absolute count value.
    pbio_error_t err = pbdrv_counter_lpf2_get_count(dev, count);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (priv->last_abs_pos == PBDRV_COUNTER_UNKNOWN_ABS_ANGLE) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    // Convert last absolute angle to signed angle in (-180, 179).
    *count = (priv->last_abs_pos + 5) / 10;
    if (*count >= 180) {
        *count -= 360;
    }

    return PBIO_SUCCESS;
}

static const pbdrv_counter_funcs_t pbdrv_counter_lpf2_funcs = {
    .get_count = pbdrv_counter_lpf2_get_count,
    .get_abs_count = pbdrv_counter_lpf2_get_abs_count,
};

void pbdrv_counter_lpf2_init(pbdrv_counter_dev_t *devs) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        const pbdrv_counter_lpf2_platform_data_t *pdata =
            &pbdrv_counter_lpf2_platform_data[i];
        private_data_t *priv = &private_data[i];
        pbdrv_counter_dev_t *dev = &devs[pdata->counter_id];

        dev->pdata = pdata;
        dev->funcs = &pbdrv_counter_lpf2_funcs;
        dev->priv = priv;
    }
}

#endif // PBDRV_CONFIG_COUNTER_LPF2
