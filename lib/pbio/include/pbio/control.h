// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_CONTROL_H_
#define _PBIO_CONTROL_H_

#include <stdint.h>
#include <stdio.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/trajectory.h>

#include <pbio/iodev.h>

/**
 * Control states
 */
typedef enum {
    /* Passive control statuses: No PID Control Active */
    PBIO_CONTROL_PASSIVE,
    PBIO_CONTROL_ERRORED,
    /* Active control statuses: PID Control Active in non-blocking manner */   
    PBIO_CONTROL_ANGLE_BACKGROUND,
    PBIO_CONTROL_TIME_BACKGROUND,
    /* Active control statuses: PID Control Active which blocks user program */  
    PBIO_CONTROL_ANGLE_FOREGROUND,
    PBIO_CONTROL_TIME_FOREGROUND,
} pbio_servo_state_t;

/**
 * Control settings
 */
typedef struct _pbio_control_settings_t {
    int32_t stall_rate_limit;       /**< If this speed cannnot be reached even with the maximum duty value (equal to stall_torque_limit), the motor is considered to be stalled */
    int32_t stall_time;             /**< Minimum stall time before the run_stalled action completes */
    int32_t max_rate;               /**< Soft limit on the reference encoder rate in all run commands */
    int32_t rate_tolerance;         /**< Allowed deviation (counts/s) from target speed. Hence, if speed target is zero, any speed below this tolerance is considered to be standstill. */
    int32_t count_tolerance;        /**< Allowed deviation (counts) from target before motion is considered complete */
    int32_t abs_acceleration;       /**< Encoder acceleration/deceleration rate when beginning to move or stopping. Positive value in counts per second per second */
    int32_t tight_loop_time;        /**< When a run function is called twice in this interval, assume that the user is doing their own speed control.  */
    int16_t pid_kp;                 /**< Proportional position control constant (and integral speed control constant) */
    int16_t pid_ki;                 /**< Integral position control constant */
    int16_t pid_kd;                 /**< Derivative position control constant (and proportional speed control constant) */
    int32_t max_control;            /**< Upper limit on control output */
} pbio_control_settings_t;


typedef enum {
    /**< Motor is not stalled */
    STALLED_NONE = 0x00,
    /**< The proportional duty control term is larger than the maximum and still the motor moves slower than specified limit */
    STALLED_PROPORTIONAL = 0x01,
    /**< The integral duty control term is larger than the maximum and still the motor moves slower than specified limit */
    STALLED_INTEGRAL = 0x02,
} pbio_control_stalled_t;

/**
 * Motor PID control status
 */
typedef struct _pbio_control_status_angular_t {
    bool ref_time_running;         /**< Whether the time at which the reference is evaluated is progressing (true) or paused (false) */
    count_t err_integral;          /**< Integral of position error */
    count_t count_err_prev;        /**< Position error in the previous control iteration */
    ustime_t time_prev;            /**< Time at the previous control iteration */
    ustime_t time_paused;          /**< The amount of time the speed integrator has spent paused */
    ustime_t time_stopped;         /**< Time at which the time was paused */
} pbio_control_status_angular_t;

typedef struct _pbio_control_status_timed_t {
    bool speed_integrator_running;   /**< Whether the speed integrator is active (true) or paused to prevent windup (false) */
    count_t speed_integrator;        /**< State of the speed integrator */
    ustime_t integrator_time_stopped;/**< Time at which the speed integrator last stopped */
    count_t integrator_ref_start;    /**< Integrated speed value prior to enabling integrator */
    count_t integrator_start;        /**< Integrated reference speed value prior to enabling integrator */
} pbio_control_status_timed_t;

typedef enum {
    PBIO_MOTOR_STOP_COAST,      /**< Coast the motor */
    PBIO_MOTOR_STOP_BRAKE,      /**< Brake the motor */
    PBIO_MOTOR_STOP_HOLD,       /**< Actively hold the motor in place */
    PBIO_ACTUATION_DUTY,
} pbio_control_after_stop_t; // TODO: generalize enum names to pbio_actuation_t

typedef enum {
    RUN,
    RUN_TIME,
    RUN_STALLED,
    RUN_TARGET,
    TRACK_TARGET,
} pbio_control_action_t;

typedef struct _pbio_control_t {
    pbio_control_settings_t settings;
    pbio_control_action_t action;
    pbio_control_after_stop_t after_stop;
    pbio_control_trajectory_t trajectory;
    pbio_control_status_angular_t status_angular;
    pbio_control_status_timed_t status_timed;
    pbio_control_stalled_t stalled;
} pbio_control_t;

pbio_error_t pbio_control_set_limits(pbio_control_settings_t *settings,
                                     fix16_t counts_per_output_unit,
                                     int32_t max_speed,
                                     int32_t acceleration);

pbio_error_t pbio_control_set_pid_settings(pbio_control_settings_t *settings,
                                           fix16_t counts_per_output_unit,
                                           int16_t pid_kp,
                                           int16_t pid_ki,
                                           int16_t pid_kd,
                                           int32_t tight_loop_time,
                                           int32_t position_tolerance,
                                           int32_t speed_tolerance,
                                           int32_t stall_speed_limit,
                                           int32_t stall_time);

void control_init_angle_target(pbio_control_t *ctl);
void control_init_time_target(pbio_control_t *ctl);
pbio_error_t control_update_angle_target(pbio_control_t *ctl, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_control_after_stop_t *actuation_type, int32_t *control);
pbio_error_t control_update_time_target(pbio_control_t *ctl, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_control_after_stop_t *actuation_type, int32_t *control);

#endif // _PBIO_CONTROL_H_
