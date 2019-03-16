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
 * Motor control active state
 */
typedef enum {
    /* Passive control statuses: No PID Control Active */
    PBIO_MOTOR_CONTROL_COASTING,
    PBIO_MOTOR_CONTROL_BRAKING,
    PBIO_MOTOR_CONTROL_USRDUTY,
    PBIO_MOTOR_CONTROL_ERRORED,
    /* Active control statuses: PID Control Active */   
    PBIO_MOTOR_CONTROL_TRACKING,      /**< Motor is tracking a position or holding position after completing command */
    PBIO_MOTOR_CONTROL_RUNNING_TIME,  /**< Motor is executing angle based maneuver by doing speed/position control */
    PBIO_MOTOR_CONTROL_RUNNING_ANGLE, /**< Motor is executing time based  maneuver by doing speed/position control */
} pbio_motor_control_state_t;

/**
 * Settings for an encoded motor
 */
typedef struct _pbio_motor_settings_t {
    float_t counts_per_unit;        /**< Encoder counts per output unit. Counts per degree for rotational motors, counts per cm for a linear motor. */
    float_t counts_per_output_unit; /**< Encoder counts per output unit, including optional gear train. Equals counts_per_unit*gear_ratio. */
    int32_t duty_offset;            /**< TODO. */
    int32_t max_duty_steps;         /**< TODO. */
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
} pbio_motor_settings_t;

/**
 * Status of the anti-windup integrators
 */
typedef enum {
    /**< Anti-windup status for PID position control:
         Pause the position and speed trajectory when
         the motor is stalled by pausing time. */
    TIME_PAUSED,
    TIME_RUNNING,
    /**< Anti-windup status for PI speed control:
         Pause the integration of the
         accumulated speed error when stalled. */
    SPEED_INTEGRATOR_RUNNING,
    SPEED_INTEGRATOR_PAUSED,
} windup_status_t;

typedef enum {
    /**< Motor is not stalled */
    STALLED_NONE = 0x00,
    /**< The proportional duty control term is larger than the maximum and still the motor moves slower than specified limit */
    STALLED_PROPORTIONAL = 0x01,
    /**< The integral duty control term is larger than the maximum and still the motor moves slower than specified limit */
    STALLED_INTEGRAL = 0x02,
} stalled_status_t;

/**
 * Motor control actions
 */
typedef enum {
    RUN,
    RUN_TIME,
    RUN_STALLED,
    RUN_TARGET,
    TRACK_TARGET,
} pbio_motor_action_t;

/**
 * Motor PID control status
 */
typedef struct _pbio_motor_angular_control_status_t {
    windup_status_t windup_status; /**< State of the anti-windup variables */
    count_t err_integral;          /**< Integral of position error (RUN_TARGET) */
    count_t speed_integrator;      /**< State of the speed integrator (all other modes) */
    duty_t load_duty;
    count_t count_err_prev;        /**< Position error in the previous control iteration */
    ustime_t time_prev;            /**< Time at the previous control iteration */
    ustime_t time_paused;          /**< The amount of time the speed integrator has spent paused */
    ustime_t time_stopped;         /**< Time at which the time was paused */
} pbio_motor_angular_control_status_t;

typedef struct _pbio_motor_timed_control_status_t {
    windup_status_t windup_status; /**< State of the anti-windup variables */
    count_t speed_integrator;      /**< State of the speed integrator (all other modes) */
    duty_t load_duty;
    ustime_t integrator_time_stopped;         /**< Time at which the speed integrator last stopped */
    count_t integrator_ref_start;  /**< Integrated speed value prior to enabling integrator */
    count_t integrator_start;      /**< Integrated reference speed value prior to enabling integrator */
} pbio_motor_timed_control_status_t;

/**
 * Single motor maneuver
 */
typedef struct _pbio_motor_maneuver_t {
    pbio_motor_action_t action;         /**<  Motor action type */
    pbio_motor_after_stop_t after_stop; /**<  BRAKE, COAST or HOLD after motion complete */
    pbio_motor_trajectory_t trajectory;
} pbio_motor_maneuver_t;

typedef struct _pbio_motor_t {
    pbio_motor_dir_t direction;
    bool has_encoders;
    pbio_motor_settings_t settings;
    pbio_motor_control_state_t state;
    pbio_motor_maneuver_t maneuver;
    pbio_motor_angular_control_status_t angular_control_status;
    pbio_motor_timed_control_status_t timed_control_status;
    stalled_status_t stalled;
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

pbio_error_t pbio_motor_run_time(pbio_port_t port, int32_t speed, int32_t duration, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_motor_run_until_stalled(pbio_port_t port, int32_t speed, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_motor_run_angle(pbio_port_t port, int32_t speed, int32_t angle, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_motor_run_target(pbio_port_t port, int32_t speed, int32_t target, pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_motor_track_target(pbio_port_t port, int32_t target);

#ifdef PBIO_CONFIG_ENABLE_MOTORS
void _pbio_motorcontrol_poll(void);
#else
static inline void _pbio_motorcontrol_poll(void) { }
#endif // PBIO_CONFIG_ENABLE_MOTORS

/** @}*/

#endif // _PBIO_ENCMOTOR_H_
