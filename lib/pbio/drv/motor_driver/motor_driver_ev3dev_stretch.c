// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_EV3DEV_STRETCH

#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/port.h>

#include <ev3dev_stretch/lego_motor.h>

struct _pbdrv_motor_driver_dev_t {
    pbio_port_id_t port;
};

static pbdrv_motor_driver_dev_t motor_ports[PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV];

pbio_error_t pbdrv_motor_driver_get_dev(uint8_t id, pbdrv_motor_driver_dev_t **driver) {
    if (id >= PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    *driver = &motor_ports[id];

    // if port has not been set, then driver has not been intialized
    if ((*driver)->port == 0) {
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_motor_driver_coast(pbdrv_motor_driver_dev_t *driver) {
    return ev3dev_motor_stop(driver->port);
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbdrv_motor_driver_dev_t *driver, int16_t duty_cycle) {
    return ev3dev_motor_run(driver->port, 100 * duty_cycle / PBDRV_MOTOR_DRIVER_MAX_DUTY);
}

void pbdrv_motor_driver_init(void) {
    for (int i = 0; i < PBDRV_CONFIG_MOTOR_DRIVER_NUM_DEV; i++) {
        motor_ports[i].port = PBDRV_CONFIG_FIRST_MOTOR_PORT + i;
    }
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_EV3DEV_STRETCH
