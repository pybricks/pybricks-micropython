// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LEGODEV_VIRTUAL

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>

#include <pbdrv/gpio.h>

#include <pbsys/status.h>

#include <pbio/angle.h>
#include <pbio/dcmotor.h>
#include <pbio/error.h>
#include <pbdrv/legodev.h>
#include <pbio/port.h>

#include <pbdrv/counter.h>
#include <pbdrv/legodev.h>

#include "legodev_virtual.h"
#include "../motor_driver/motor_driver_virtual_simulation.h"

struct _pbdrv_legodev_dev_t {
    const pbdrv_legodev_virtual_platform_data_t *pdata;
};

static pbdrv_legodev_dev_t devs[PBDRV_CONFIG_LEGODEV_VIRTUAL_NUM_DEV];

void pbdrv_legodev_init(void) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_LEGODEV_VIRTUAL_NUM_DEV; i++) {
        pbdrv_legodev_dev_t *legodev = &devs[i];
        legodev->pdata = &pbdrv_legodev_virtual_platform_data[i];
    }
}

pbio_error_t pbdrv_legodev_get_motor_index(pbdrv_legodev_dev_t *legodev, uint8_t *index) {
    *index = legodev->pdata->motor_driver_index;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    pbdrv_motor_driver_dev_t *motor_driver;
    pbio_error_t err = pbdrv_motor_driver_get_dev(legodev->pdata->motor_driver_index, &motor_driver);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    pbdrv_motor_driver_virtual_simulation_get_angle(motor_driver, &angle->rotations, &angle->millidegrees);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_abs_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {

    pbio_error_t err = pbdrv_legodev_get_angle(legodev, angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Assumes rotations are flushed, which above function takes care of.
    if (angle->rotations != 0) {
        return PBIO_ERROR_FAILED;
    }

    // Convert absolute angle to signed angle in (-180000, 179999).
    if (angle->millidegrees >= 180000) {
        angle->millidegrees -= 360000;
    }
    return PBIO_SUCCESS;
}

static bool type_id_matches(pbdrv_legodev_type_id_t *type, pbdrv_legodev_type_id_t actual_type) {

    // Returns what was actually detected.
    pbdrv_legodev_type_id_t desired_type = *type;
    *type = actual_type;

    // Pass for any dc motor.
    if (desired_type == PBDRV_LEGODEV_TYPE_ID_ANY_DC_MOTOR) {
        return actual_type == PBDRV_LEGODEV_TYPE_ID_LPF2_MMOTOR || actual_type == PBDRV_LEGODEV_TYPE_ID_LPF2_TRAIN;
    }

    // Pass for any encoded motor.
    if (desired_type == PBDRV_LEGODEV_TYPE_ID_ANY_ENCODED_MOTOR) {
        return actual_type == PBDRV_LEGODEV_TYPE_ID_INTERACTIVE_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_SPIKE_M_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_SPIKE_L_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_SPIKE_S_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_TECHNIC_M_ANGULAR_MOTOR ||
               actual_type == PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR;
    }

    // Pass for any LEGO UART device.
    if (desired_type == PBDRV_LEGODEV_TYPE_ID_ANY_LUMP_UART) {
        return actual_type > PBDRV_LEGODEV_TYPE_ID_LPF2_UNKNOWN_UART && actual_type <= PBDRV_LEGODEV_TYPE_ID_TECHNIC_L_ANGULAR_MOTOR;
    }

    // Require an exact match.
    return desired_type == actual_type;
}

pbio_error_t pbdrv_legodev_get_device(pbio_port_id_t port_id, pbdrv_legodev_type_id_t *type_id, pbdrv_legodev_dev_t **legodev) {
    for (uint8_t i = 0; i < PBIO_ARRAY_SIZE(devs); i++) {
        pbdrv_legodev_dev_t *candidate = &devs[i];
        if (candidate->pdata->port_id == port_id) {
            *legodev = candidate;
            return type_id_matches(type_id, candidate->pdata->type_id) ? PBIO_SUCCESS : PBIO_ERROR_NO_DEV;
        }
    }
    return PBIO_ERROR_NO_DEV;
}

bool pbdrv_legodev_needs_permanent_power(pbdrv_legodev_dev_t *legodev) {
    return false;
}

pbio_error_t pbdrv_legodev_get_info(pbdrv_legodev_dev_t *legodev, pbdrv_legodev_info_t **info) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_legodev_is_ready(pbdrv_legodev_dev_t *legodev) {
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_set_mode(pbdrv_legodev_dev_t *legodev, uint8_t mode) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_legodev_set_mode_with_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, const void *data, uint8_t size) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_legodev_get_data(pbdrv_legodev_dev_t *legodev, uint8_t mode, void **data) {
    *data = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_LEGODEV_VIRTUAL
