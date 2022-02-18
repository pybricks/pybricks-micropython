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
    ev3dev_motor_t *mtr;
    pbio_error_t err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return ev3dev_motor_stop(mtr);
}

pbio_error_t pbdrv_motor_set_duty_cycle(pbio_port_id_t port, int16_t duty_cycle) {
    ev3dev_motor_t *mtr;
    pbio_error_t err = ev3dev_motor_get(&mtr, port);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return ev3dev_motor_run(mtr, duty_cycle / 100);
}

#endif // PBDRV_CONFIG_MOTOR
