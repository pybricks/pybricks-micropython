// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020,2022 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_MOTOR && !PBIO_TEST_BUILD

#include <pbdrv/motor.h>
#include <pbio/config.h>
#include <pbio/iodev.h>

#include <ev3dev_stretch/lego_motor.h>
#include <ev3dev_stretch/lego_port.h>

pbio_error_t pbdrv_motor_coast(pbio_port_id_t port) {
    return ev3dev_motor_stop(port);
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    return ev3dev_motor_run(port, duty_cycle / 100);
}

#endif // PBDRV_CONFIG_MOTOR
