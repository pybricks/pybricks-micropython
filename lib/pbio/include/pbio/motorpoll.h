// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 Laurens Valk
// Copyright (c) 2020 LEGO System A/S

#ifndef _PBIO_MOTORPOLL_H_
#define _PBIO_MOTORPOLL_H_

#include <pbio/drivebase.h>
#include <pbio/error.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

pbio_error_t pbio_servo_get(pbio_port_t port, pbio_servo_t **srv);
pbio_error_t pbio_drivebase_get(pbio_drivebase_t **db);

void _pbio_motorpoll_reset_all(void);
void _pbio_motorpoll_poll(void);

#else

static inline void _pbio_motorpoll_reset_all(void) { }
static inline void _pbio_motorpoll_poll(void) { }

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER

#endif // _PBIO_MOTORPOLL_H_
