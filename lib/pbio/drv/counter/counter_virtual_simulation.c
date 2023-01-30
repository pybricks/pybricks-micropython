// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Virtual counter driver

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER_VIRTUAL_SIMULATION

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include <pbio/util.h>
#include <pbdrv/motor_driver.h>
#include "../motor_driver/motor_driver_virtual_simulation.h"
#include "counter.h"

#define DEBUG 0
#if DEBUG
#define dbg_err(s) perror(s)
#else
#define dbg_err(s)
#endif

typedef struct {
    pbdrv_counter_dev_t *dev;
    pbdrv_motor_driver_dev_t *motor_driver;
    uint8_t index;
} private_data_t;

static private_data_t private_data[PBDRV_CONFIG_COUNTER_VIRTUAL_SIMULATION_NUM_DEV];

static pbio_error_t pbdrv_counter_virtual_simulation_get_angle(pbdrv_counter_dev_t *dev, int32_t *rotations, int32_t *millidegrees) {
    private_data_t *priv = dev->priv;
    pbdrv_motor_driver_virtual_simulation_get_angle(priv->motor_driver, rotations, millidegrees);
    return PBIO_SUCCESS;
}

static pbio_error_t pbdrv_counter_virtual_simulation_get_abs_angle(pbdrv_counter_dev_t *dev, int32_t *millidegrees) {

    int32_t rotations;
    private_data_t *priv = dev->priv;
    pbdrv_motor_driver_virtual_simulation_get_angle(priv->motor_driver, &rotations, millidegrees);

    // Assumes rotations are flushed, which above function takes care of.
    if (rotations != 0) {
        return PBIO_ERROR_FAILED;
    }

    // Convert absolute angle to signed angle in (-180000, 179999).
    if (*millidegrees >= 180000) {
        *millidegrees -= 360000;
    }
    return PBIO_SUCCESS;
}

static const pbdrv_counter_funcs_t pbdrv_counter_virtual_simulation_funcs = {
    .get_angle = pbdrv_counter_virtual_simulation_get_angle,
    .get_abs_angle = pbdrv_counter_virtual_simulation_get_abs_angle,
};

void pbdrv_counter_virtual_simulation_init(pbdrv_counter_dev_t *devs) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(private_data); i++) {
        private_data_t *priv = &private_data[i];

        priv->index = i;

        // FIXME: assuming that these are the only counter devices
        // counter_id should be passed from platform data instead
        // i.e. enumerate Simulation `platform.counter.keys()`.
        _Static_assert(PBDRV_CONFIG_COUNTER_VIRTUAL_SIMULATION_NUM_DEV == PBDRV_CONFIG_COUNTER_NUM_DEV,
            "need to fix counter_virtual_simulation implementation to allow other counter devices");

        priv->dev = &devs[i];
        priv->dev->funcs = &pbdrv_counter_virtual_simulation_funcs;
        priv->dev->priv = priv;

        pbdrv_motor_driver_get_dev(i, &priv->motor_driver);
    }
}

#endif // PBDRV_CONFIG_COUNTER_VIRTUAL_SIMULATION
