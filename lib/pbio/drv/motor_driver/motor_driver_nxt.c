// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020,2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_NXT

#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/error.h>
#include <pbio/port.h>

#include <nxt/nxt_motors.h>

pbio_error_t pbdrv_motor_driver_coast(pbio_port_id_t port) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    nxt_motor_set_speed(port - PBDRV_CONFIG_FIRST_MOTOR_PORT, 0, 0);

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    nxt_motor_set_speed(port - PBDRV_CONFIG_FIRST_MOTOR_PORT, 100 * duty_cycle / PBDRV_MOTOR_DRIVER_MAX_DUTY, 1);

    return PBIO_SUCCESS;
}

void pbdrv_motor_driver_init(void) {
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_NXT
