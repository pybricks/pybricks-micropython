// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR && !PBIO_TEST_BUILD

#include <stdbool.h>

#include <pbdrv/motor.h>
#include <pbio/config.h>

#include <nxt/nxt_motors.h>

pbio_error_t pbdrv_motor_coast(pbio_port_id_t port) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    nxt_motor_set_speed(port - PBDRV_CONFIG_FIRST_MOTOR_PORT, 0, 0);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    nxt_motor_set_speed(port - PBDRV_CONFIG_FIRST_MOTOR_PORT, duty_cycle / 100, 1);
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_get_id(pbio_port_id_t port, pbio_iodev_type_id_t *id) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    *id = PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR;
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_setup(pbio_port_id_t port, bool is_servo) {
    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_MOTOR
