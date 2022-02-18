// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#ifndef _PBIO_LEGO_MOTOR_H_
#define _PBIO_LEGO_MOTOR_H_

#include <stdbool.h>

#include <pbio/error.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

typedef struct _ev3dev_motor_t ev3dev_motor_t;

pbio_error_t ev3dev_motor_get(ev3dev_motor_t **mtr, pbio_port_id_t port);
pbio_error_t ev3dev_motor_setup(pbio_port_id_t port, bool is_servo);
pbio_error_t ev3dev_motor_get_id(pbio_port_id_t port, pbio_iodev_type_id_t *id);
pbio_error_t ev3dev_motor_run(ev3dev_motor_t *mtr, int duty_cycle);
pbio_error_t ev3dev_motor_stop(ev3dev_motor_t *mtr);

#endif // _PBIO_LEGO_MOTOR_H_
