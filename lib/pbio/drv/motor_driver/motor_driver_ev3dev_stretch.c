// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR_DRIVER_EV3DEV_STRETCH

#include <stdint.h>

#include <pbdrv/motor_driver.h>
#include <pbio/port.h>

#include <ev3dev_stretch/lego_motor.h>

pbio_error_t pbdrv_motor_driver_coast(pbio_port_id_t port) {
    return ev3dev_motor_stop(port);
}

pbio_error_t pbdrv_motor_driver_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    return ev3dev_motor_run(port, 100 * duty_cycle / PBDRV_MOTOR_DRIVER_MAX_DUTY);
}

void pbdrv_motor_driver_init(void) {
}

#endif // PBDRV_CONFIG_MOTOR_DRIVER_EV3DEV_STRETCH
