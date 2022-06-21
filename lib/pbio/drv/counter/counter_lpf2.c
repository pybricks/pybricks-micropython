// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// LPF2 Counter driver
//
// This driver parses type-specific position readouts from LPF2 motors.
//
// This driver currently requires that pbdrv_counter_lpf2_get_angle gets
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
    pbdrv_counter_dev_t *dev;
    /**
     * Whether the motor can measure an absolute angle.
     */
    bool supports_abs_angle;
    /**
     * Millidegrees within the circle, 0--360 000
     */
    int32_t millidegrees;
    /**
     * Whole rotation counter. At 1000 deg/s, overflows after 24 years.
     */
    int32_t rotations;
} private_data_t;

static private_data_t private_data[PBDRV_CONFIG_COUNTER_LPF2_NUM_DEV];

// Parses latest LPF2 message and stores result in private data.
static pbio_error_t pbdrv_counter_lpf2_update(pbdrv_counter_dev_t *dev) {

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

    // Check if device can measure absolute angles.
    priv->supports_abs_angle = PBIO_IODEV_IS_ABS_MOTOR(iodev);

    // Ensure that we are on expected mode.
    uint8_t mode_id = priv->supports_abs_angle ?
        PBIO_IODEV_MODE_PUP_ABS_MOTOR__CALIB:
        PBIO_IODEV_MODE_PUP_REL_MOTOR__POS;
    if (iodev->mode != mode_id) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get pointer to LPF2 data buffer.
    uint8_t *data;
    err = pbio_iodev_get_data(iodev, &data);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // For incremental encoders, we read data in degrees.
    if (!priv->supports_abs_angle) {
        // At max speed (1000 deg/s), degrees overflows after 24 days, so we
        // don't bother with additional buffering and resetting.
        int32_t degrees = pbio_get_uint32_le(data);

        // Store as (rotations, millidegree) pair. This is slightly redundant,
        // but this makes it easy to deal with both motor types consistently.
        priv->millidegrees = (degrees % 360) * 1000;
        priv->rotations = degrees / 360;
        return PBIO_SUCCESS;
    }

    // For absolute encoders, we need to keep track of whole rotations.
    // First, read value in tenths of degrees.
    int32_t abs_now = (int16_t)pbio_get_uint16_le(data + 2);
    int32_t abs_prev = priv->millidegrees / 100;

    // Store measured millidegree state value.
    priv->millidegrees = abs_now * 100;

    // Update rotation counter as encoder passes through 0
    if (abs_prev > 2700 && abs_now < 900) {
        priv->rotations += 1;
    }
    if (abs_prev < 900 && abs_now > 2700) {
        priv->rotations -= 1;
    }
    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_counter_lpf2_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {

    // Read sensor to update position buffer.
    pbio_error_t err = pbdrv_counter_lpf2_update(dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return total angle.
    private_data_t *priv = dev->priv;
    *rotations = priv->rotations;
    *millidegrees = priv->millidegrees;
    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_counter_lpf2_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {

    private_data_t *priv = dev->priv;

    // Read sensor to update position buffer.
    pbio_error_t err = pbdrv_counter_lpf2_update(dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return error if absolute angle not available.
    if (!priv->supports_abs_angle) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }

    // Convert absolute angle to signed angle in (-180000, 179999).
    *millidegrees = priv->millidegrees;
    if (*millidegrees >= 180000) {
        *millidegrees -= 360000;
    }
    return PBIO_SUCCESS;
}

static const pbdrv_counter_funcs_t pbdrv_counter_lpf2_funcs = {
    .get_angle = pbdrv_counter_lpf2_get_angle,
    .get_abs_angle = pbdrv_counter_lpf2_get_abs_angle,
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
