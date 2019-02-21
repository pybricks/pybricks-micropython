// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef _PBIO_MOTORCONTROL_H_
#define _PBIO_MOTORCONTROL_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>

#include <pbio/motor.h>

/**
 * \addtogroup Motor Motors
 * @{
 */



/**
 * Busy wait (or not) for a run command to complete
 */
typedef enum {
    PBIO_MOTOR_RUN_FOREGROUND = true, /**< Execute run command in the foreground and pause user program until run command is complete */
    PBIO_MOTOR_RUN_BACKGROUND = false,/**< Execute run command in the background and proceed with user program */
} pbio_motor_run_t;

pbio_error_t pbio_encmotor_is_stalled(pbio_port_t port, bool *stalled);

pbio_error_t pbio_encmotor_run(pbio_port_t port, int32_t speed);

pbio_error_t pbio_encmotor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_time(pbio_port_t port, int32_t speed, int32_t duration, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_until_stalled(pbio_port_t port, int32_t speed, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_angle(pbio_port_t port, int32_t speed, int32_t angle, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run_target(pbio_port_t port, int32_t speed, int32_t target, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_track_target(pbio_port_t port, int32_t target);

#ifdef PBIO_CONFIG_ENABLE_MOTORS
void _pbio_motorcontrol_poll(void);
#else
static inline void _pbio_motorcontrol_poll(void) { }
#endif // PBIO_CONFIG_ENABLE_MOTORS

/** @}*/

#endif // _PBIO_MOTORCONTROL_H_
