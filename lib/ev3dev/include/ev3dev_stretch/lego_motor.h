// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _PBIO_LEGO_MOTOR_H_
#define _PBIO_LEGO_MOTOR_H_

#include <stdbool.h>

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

pbio_error_t ev3dev_motor_setup(pbio_port_id_t port, bool is_servo);
pbio_error_t ev3dev_motor_get_id(pbio_port_id_t port, pbio_iodev_type_id_t *id);
pbio_error_t ev3dev_motor_run(pbio_port_id_t port, int duty_cycle);
pbio_error_t ev3dev_motor_stop(pbio_port_id_t port);

#endif // _PBIO_LEGO_MOTOR_H_
