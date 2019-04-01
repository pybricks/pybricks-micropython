// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef _PBIO_ENCMOTOR_H_
#define _PBIO_ENCMOTOR_H_

#include <stdint.h>
#include <stdio.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/motorref.h>
#include <pbio/control.h>

#include <pbio/iodev.h>

/**
 * \addtogroup Motor Motors
 * @{
 */

typedef float float_t;


#define PORT_TO_IDX(p) ((p) - PBDRV_CONFIG_FIRST_MOTOR_PORT)
#define MAX_DCMOTOR_SETTINGS_STR_LENGTH (128)

#define MAX_ENCMOTOR_SETTINGS_STR_LENGTH (400)

#define PBIO_DUTY_STEPS (PBDRV_MAX_DUTY)
#define PBIO_DUTY_USER_STEPS (100)
#define PBIO_DUTY_STEPS_PER_USER_STEP (PBIO_DUTY_STEPS/PBIO_DUTY_USER_STEPS)

/**
 * Motor direction convention
 */
typedef enum {
    PBIO_MOTOR_DIR_CLOCKWISE,         /**< Positive speed/duty means clockwise */
    PBIO_MOTOR_DIR_COUNTERCLOCKWISE,  /**< Positive speed/duty means counterclockwise */
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
 * Single motor actions
 */
typedef enum {
    RUN,
    RUN_TIME,
    RUN_STALLED,
    RUN_TARGET,
    TRACK_TARGET,
} pbio_motor_action_t;

/**
 * Single motor maneuver
 */
typedef struct _pbio_motor_maneuver_t {
    pbio_motor_action_t action;         /**<  Motor action type */
    pbio_motor_after_stop_t after_stop; /**<  BRAKE, COAST or HOLD after motion complete */
    pbio_motor_trajectory_t trajectory;
} pbio_motor_maneuver_t;

typedef struct _pbio_control_t {
    pbio_control_settings_t settings;
    pbio_motor_maneuver_t maneuver;
    pbio_status_angular_t status_angular;
    pbio_status_timed_t status_timed;
    stalled_status_t stalled;
} pbio_control_t;

typedef struct _pbio_motor_t {
    pbio_motor_dir_t direction;
    int32_t offset;                 /**< Virtual zero point of the encoder */
    float_t counts_per_unit;        /**< Encoder counts per output unit. Counts per degree for rotational motors, counts per cm for a linear motor. */
    float_t counts_per_output_unit; /**< Encoder counts per output unit, including optional gear train. Equals counts_per_unit*gear_ratio. */
    int32_t duty_offset;            /**< TODO. */
    int32_t max_duty_steps;         /**< TODO. */
    bool has_encoders;
    pbio_motor_state_t state;
    pbio_control_t control;
} pbio_motor_t;

pbio_motor_t motor[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_motor_set_dc_settings(pbio_port_t port, int32_t stall_torque_limit_pct, int32_t duty_offset_pct);

pbio_error_t pbio_motor_get_dc_settings(pbio_port_t port, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct);

pbio_error_t pbio_motor_set_run_settings(pbio_port_t port, int32_t max_speed, int32_t acceleration);

pbio_error_t pbio_motor_set_pid_settings(pbio_port_t port, int16_t pid_kp, int16_t pid_ki, int16_t pid_kd, int32_t tight_loop_time, int32_t position_tolerance, int32_t speed_tolerance, int32_t stall_speed_limit, int32_t stall_time);

pbio_error_t pbio_motor_coast(pbio_port_t port);

pbio_error_t pbio_motor_brake(pbio_port_t port);

pbio_error_t pbio_motor_set_duty_cycle_sys(pbio_port_t port, int32_t duty_steps);

pbio_error_t pbio_motor_set_duty_cycle_usr(pbio_port_t port, float_t duty_steps);

pbio_error_t pbio_motor_setup(pbio_port_t port, pbio_motor_dir_t direction, float_t gear_ratio);

void pbio_motor_print_settings(pbio_port_t port, char *dc_settings_string, char *enc_settings_string);

bool pbio_motor_has_encoder(pbio_port_t port);

pbio_error_t pbio_motor_get_encoder_count(pbio_port_t port, int32_t *count);

pbio_error_t pbio_motor_reset_encoder_count(pbio_port_t port, int32_t reset_count);

pbio_error_t pbio_motor_get_angle(pbio_port_t port, int32_t *angle);

pbio_error_t pbio_motor_reset_angle(pbio_port_t port, int32_t reset_angle);

pbio_error_t pbio_motor_get_encoder_rate(pbio_port_t port, int32_t *encoder_rate);

pbio_error_t pbio_motor_get_angular_rate(pbio_port_t port, int32_t *angular_rate);

pbio_error_t pbio_motor_is_stalled(pbio_port_t port, bool *stalled);

pbio_error_t pbio_motor_run(pbio_port_t port, int32_t speed);

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_motor_run_time(pbio_port_t port, int32_t speed, int32_t duration, pbio_motor_after_stop_t after_stop, bool foreground);

pbio_error_t pbio_motor_run_until_stalled(pbio_port_t port, int32_t speed, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_motor_run_angle(pbio_port_t port, int32_t speed, int32_t angle, pbio_motor_after_stop_t after_stop, bool foreground);

pbio_error_t pbio_motor_run_target(pbio_port_t port, int32_t speed, int32_t target, pbio_motor_after_stop_t after_stop, bool foreground);

pbio_error_t pbio_motor_track_target(pbio_port_t port, int32_t target);

#ifdef PBIO_CONFIG_ENABLE_MOTORS
void _pbio_motorcontrol_poll(void);
#else
static inline void _pbio_motorcontrol_poll(void) { }
#endif // PBIO_CONFIG_ENABLE_MOTORS

/** @}*/

#endif // _PBIO_ENCMOTOR_H_
