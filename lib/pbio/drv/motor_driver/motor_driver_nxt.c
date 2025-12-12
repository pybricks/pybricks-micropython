// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020,2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_NXT

#include <stdbool.h>
#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/error.h>

#include "../rproc/rproc_nxt.h"

struct _pbdrv_motor_driver_dev_t {
    /**
     * True if the motor driver supports PWM (if it is a motor port)
     */
    bool supports_pwm;
    /**
     * Driver specific index.
     */
    uint8_t index;
};

static pbdrv_motor_driver_dev_t motor_ports[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV] = {
    // Motor A.
    [0] = {
        .supports_pwm = true,
        .index = 0,
    },
    // Motor B.
    [1] = {
        .supports_pwm = true,
        .index = 1,
    },
    // Motor C.
    [2] = {
        .supports_pwm = true,
        .index = 2,
    },
    // Sensor port 1 (no PWM).
    [3] = {
        .supports_pwm = false,
        .index = 0,
    },
    // Sensor port 2 (no PWM).
    [4] = {
        .supports_pwm = false,
        .index = 1,
    },
    // Sensor port 3 (no PWM).
    [5] = {
        .supports_pwm = false,
        .index = 2,
    },
    // Sensor port 4 (no PWM).
    [6] = {
        .supports_pwm = false,
        .index = 3,
    }
};

pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    if (id >= PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *driver = &motor_ports[id];
    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {
    if (driver->supports_pwm) {
        return pbdrv_rproc_nxt_set_duty_cycle(driver->index, 0, false);
    }
    return pbdrv_rproc_nxt_set_sensor_power(driver->index, false);
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    if (driver->supports_pwm) {
        return pbdrv_rproc_nxt_set_duty_cycle(driver->index, 100 * duty_cycle / PBDRV_MOTOR_DRIVER_MAX_DUTY, true);
    }

    // Set sensor power only if 100% reverse duty cycle is requested.
    return pbdrv_rproc_nxt_set_sensor_power(driver->index, duty_cycle == -PBDRV_MOTOR_DRIVER_MAX_DUTY);
}

void pbdrv_motor_driver_init(void) {
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_NXT
