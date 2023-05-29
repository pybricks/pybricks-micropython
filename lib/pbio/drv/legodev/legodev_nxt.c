// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LEGODEV_NXT

#include <stdbool.h>

#include <pbdrv/counter.h>
#include <pbdrv/legodev.h>

#include <nxos/drivers/motors.h>

#include "legodev_nxt.h"

struct _pbdrv_legodev_dev_t {
    bool is_motor;
    union {
        const pbdrv_legodev_nxt_motor_platform_data_t *motor;
        const pbdrv_legodev_nxt_sensor_platform_data_t *sensor;
    };
};

static pbdrv_legodev_dev_t devs[PBDRV_CONFIG_LEGODEV_NXT_NUM_MOTOR + PBDRV_CONFIG_LEGODEV_NXT_NUM_SENSOR];

void pbdrv_legodev_init(void) {
    for (uint8_t i = 0; i < PBDRV_CONFIG_LEGODEV_NXT_NUM_MOTOR; i++) {
        // Initialize common device type as external.
        pbdrv_legodev_dev_t *legodev = &devs[i];
        legodev->is_motor = true;
        legodev->motor = &pbdrv_legodev_nxt_motor_platform_data[i];
    }

    for (uint8_t i = 0; i < PBDRV_CONFIG_LEGODEV_NXT_NUM_SENSOR; i++) {
        // Initialize common device type as external.
        pbdrv_legodev_dev_t *legodev = &devs[i + PBDRV_CONFIG_LEGODEV_NXT_NUM_MOTOR];
        legodev->is_motor = false;
        legodev->sensor = &pbdrv_legodev_nxt_sensor_platform_data[i];
    }
}

pbio_error_t pbdrv_legodev_get_motor_index(pbdrv_legodev_dev_t *legodev, uint8_t *index) {
    if (!legodev->is_motor) {
        return PBIO_ERROR_NO_DEV;
    }
    *index = legodev->motor->motor_driver_index;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    if (!legodev->is_motor) {
        return PBIO_ERROR_NOT_SUPPORTED;
    }
    int32_t degrees = nx_motors_get_tach_count(legodev->motor->motor_driver_index);
    angle->rotations = degrees / 360;
    angle->millidegrees = (degrees - angle->rotations * 360) * 1000;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_legodev_get_abs_angle(pbdrv_legodev_dev_t *legodev, pbio_angle_t *angle) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

bool type_id_matches(pbdrv_legodev_type_id_t *type, pbdrv_legodev_type_id_t actual_type) {
    return false;
}

pbio_error_t pbdrv_legodev_get_device(pbio_port_id_t port_id, pbdrv_legodev_type_id_t *type_id, pbdrv_legodev_dev_t **legodev) {
    return PBIO_ERROR_NO_DEV;
}

bool pbdrv_legodev_needs_permanent_power(pbdrv_legodev_dev_t *legodev) {
    return false;
}

pbio_error_t pbdrv_legodev_get_info(pbdrv_legodev_dev_t *legodev, pbdrv_legodev_info_t **info) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

pbio_error_t pbdrv_legodev_is_ready(pbdrv_legodev_dev_t *legodev) {
    return PBIO_ERROR_NOT_SUPPORTED;
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

#endif // PBDRV_CONFIG_LEGODEV_NXT
