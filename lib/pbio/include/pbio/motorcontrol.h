/*
 * Copyright (c) 2018 Laurens Valk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
