// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_MOTOR_PROCESS_H_
#define _PBIO_MOTOR_PROCESS_H_

#include <pbio/drivebase.h>
#include <pbio/error.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

pbio_error_t pbio_motor_process_get_drivebase(pbio_drivebase_t **db);
pbio_error_t pbio_motor_process_get_servo(pbio_port_t port, pbio_servo_t **srv);

void pbio_motor_process_reset(void);

#else

static inline void pbio_motor_process_reset(void) {
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

#endif // _PBIO_MOTOR_PROCESS_H_
