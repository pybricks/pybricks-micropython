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

#ifndef _PBIO_DCMOTOR_H_
#define _PBIO_DCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/iodev.h>
#include <pbdrv/motor.h>

typedef float float_t;

#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)
#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (200)

/**
 * Motor direction convention
 */
typedef enum {
    PBIO_MOTOR_DIR_NORMAL,      /**< Use the normal motor-specific convention for the positive direction */
    PBIO_MOTOR_DIR_INVERTED,    /**< Swap positive and negative for both the encoder value and the duty cycle */
} pbio_motor_dir_t;

/**
 * Motor action executed after completing a run command that ends in a smooth stop.
 */
typedef enum {
    PBIO_MOTOR_STOP_COAST,      /**< Coast the motor */
    PBIO_MOTOR_STOP_BRAKE,      /**< Brake the motor */
    PBIO_MOTOR_STOP_HOLD,       /**< Actively hold the motor in place */
} pbio_motor_after_stop_t;

/**
 * Motor control active state
 */
typedef enum {
    PBIO_MOTOR_CONTROL_PASSIVE, /**< Motor is coasting, braking, or set to a duty value by the user. */
    PBIO_MOTOR_CONTROL_HOLDING,    /**< Motor is holding position or speed after completing command: Firmware repeatedly sets duty cycle to keep constant position */
    PBIO_MOTOR_CONTROL_RUNNING, /**< Motor busy executing command: Firmware repeatedly sets duty cycle to control motor speed and position for a desired trajectory*/     
    PBIO_MOTOR_CONTROL_STARTING, /**< Motor ready for first control update */
    PBIO_MOTOR_CONTROL_RESTARTING, /**< Motor ready to transition to first control update of new command*/   
} pbio_motor_control_active_t;

pbio_motor_control_active_t motor_control_active[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

/**
 * Settings for a DC Motor
 */
typedef struct _pbio_dcmotor_settings_t {
    pbio_motor_dir_t direction; /**< Whether or not polarity of duty cycle and encoder counter is inverted */
    int32_t max_stall_duty;     /**< Upper limit on duty cycle, which corresponds to a maximum torque while stalled. */
} pbio_dcmotor_settings_t;

pbio_dcmotor_settings_t dcmotor_settings[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_dcmotor_setup(pbio_port_t port, pbio_motor_dir_t direction);

pbio_error_t pbio_dcmotor_set_settings(pbio_port_t port, int16_t stall_torque_limit_pct);

void pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string);

pbio_error_t pbio_dcmotor_coast(pbio_port_t port);

pbio_error_t pbio_dcmotor_brake(pbio_port_t port);

pbio_error_t pbio_dcmotor_set_duty_cycle_int(pbio_port_t port, int32_t duty_cycle_int);

pbio_error_t pbio_dcmotor_set_duty_cycle(pbio_port_t port, float_t duty_cycle);

#endif // _PBIO_DCMOTOR_H_
