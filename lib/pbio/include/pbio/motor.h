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

#ifndef _PBIO_ENCMOTOR_H_
#define _PBIO_ENCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>

#include <pbio/error.h>
#include <pbio/port.h>

#include <pbio/iodev.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

typedef float float_t;


#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)
#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (200)

#define MAX_ENCMOTOR_SETTINGS_STR_LENGTH (256)
#define MS_PER_SECOND (1000)
#define US_PER_MS (1000)
#define US_PER_SECOND (1000000)

/**
 * Motor direction convention
 */
typedef enum {
    PBIO_MOTOR_DIR_NORMAL,      /**< Use the normal motor-specific convention for the positive direction */
    PBIO_MOTOR_DIR_INVERTED,    /**< Swap positive and negative for both the encoder value and the duty cycle */
} pbio_motor_dir_t;

pbio_motor_dir_t motor_directions[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

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
 * Settings for an encoded motor
 */
typedef struct _pbio_encmotor_settings_t {
    float_t counts_per_unit;        /**< Encoder counts per output unit. Counts per degree for rotational motors, counts per cm for a linear motor. */
    float_t counts_per_output_unit; /**< Encoder counts per output unit, including optional gear train. Equals counts_per_unit*gear_ratio. */
    int32_t max_stall_duty;     /**< Upper limit on duty cycle, which corresponds to a maximum torque while stalled. */
    int32_t stall_rate_limit;       /**< If this speed cannnot be reached even with the maximum duty value (equal to stall_torque_limit), the motor is considered to be stalled */
    int32_t stall_time;             /**< Minimum stall time before the run_stalled action completes */
    int32_t max_rate;               /**< Soft limit on the reference encoder rate in all run commands */
    int32_t rate_tolerance;         /**< Allowed deviation (counts/s) from target speed. Hence, if speed target is zero, any speed below this tolerance is considered to be standstill. */
    int32_t count_tolerance;        /**< Allowed deviation (counts) from target before motion is considered complete */
    int32_t abs_acceleration;       /**< Encoder acceleration/deceleration rate when beginning to move or stopping. Positive value in counts per second per second */
    int32_t tight_loop_time;        /**< When a run function is called twice in this interval, assume that the user is doing their own speed control.  */
    int32_t offset;                 /**< Virtual zero point of the encoder */
    int16_t pid_kp;                 /**< Proportional position control constant (and integral speed control constant) */
    int16_t pid_ki;                 /**< Integral position control constant */
    int16_t pid_kd;                 /**< Derivative position control constant (and proportional speed control constant) */
} pbio_encmotor_settings_t;

bool motor_has_encoders[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];
pbio_encmotor_settings_t encmotor_settings[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_encmotor_set_run_settings(pbio_port_t port, int32_t max_speed, int32_t acceleration);

pbio_error_t pbio_encmotor_set_pid_settings(pbio_port_t port, int16_t pid_kp, int16_t pid_ki, int16_t pid_kd, int32_t tight_loop_time, int32_t position_tolerance, int32_t speed_tolerance);

pbio_error_t pbio_encmotor_set_stall_settings(pbio_port_t port, int32_t stall_torque_limit_pct, int32_t stall_speed_limit, int32_t stall_time);

pbio_error_t pbio_encmotor_get_stall_settings(pbio_port_t port, int32_t *stall_torque_limit_pct, int32_t *stall_speed_limit, int32_t *stall_time);

pbio_error_t pbio_dcmotor_setup(pbio_port_t port, pbio_motor_dir_t direction);

void pbio_dcmotor_print_settings(pbio_port_t port, char *settings_string);

pbio_error_t pbio_dcmotor_coast(pbio_port_t port);

pbio_error_t pbio_dcmotor_brake(pbio_port_t port);

pbio_error_t pbio_dcmotor_set_duty_cycle_int(pbio_port_t port, int32_t duty_cycle_int);

pbio_error_t pbio_dcmotor_set_duty_cycle(pbio_port_t port, float_t duty_cycle);

pbio_error_t pbio_motor_setup(pbio_port_t port, pbio_motor_dir_t direction, float_t gear_ratio);


void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string);

bool pbio_encmotor_has_encoder(pbio_port_t port);

pbio_error_t pbio_encmotor_get_encoder_count(pbio_port_t port, int32_t *count);

pbio_error_t pbio_encmotor_reset_encoder_count(pbio_port_t port, int32_t reset_count);

pbio_error_t pbio_encmotor_get_angle(pbio_port_t port, int32_t *angle);

pbio_error_t pbio_encmotor_reset_angle(pbio_port_t port, int32_t reset_angle);

pbio_error_t pbio_encmotor_get_encoder_rate(pbio_port_t port, int32_t *encoder_rate);

pbio_error_t pbio_encmotor_get_angular_rate(pbio_port_t port, int32_t *angular_rate);

/** @}*/

#endif // _PBIO_ENCMOTOR_H_
