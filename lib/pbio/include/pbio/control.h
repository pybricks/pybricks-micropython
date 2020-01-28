// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PBIO_CONTROL_H_
#define _PBIO_CONTROL_H_

#include <stdint.h>
#include <stdio.h>

#include <fixmath.h>

#include <pbio/error.h>
#include <pbio/port.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>

#include <pbio/iodev.h>

/**
 * Control settings
 */
typedef struct _pbio_control_settings_t {
    fix16_t counts_per_unit;        /**< Conversion between user units (degree, mm, etc) and integer counts used internally by controller */
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
    PBIO_ACTUATION_COAST,      /**< Coast the motor */
    PBIO_ACTUATION_BRAKE,      /**< Brake the motor */
    PBIO_ACTUATION_HOLD,       /**< Actively hold the motor in place */
    PBIO_ACTUATION_DUTY,
} pbio_actuation_t;

// Maneuver-specific function that returns true if maneuver is done, based on current state
typedef bool (*pbio_control_done_t)(pbio_trajectory_t *trajectory,
                                    pbio_control_settings_t *settings,
                                    int32_t time,
                                    int32_t count,
                                    int32_t rate,
                                    bool stalled);

// Shortcut for a done function that says it is never done
static inline bool pbio_control_never_done(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return false;
}

// Shortcut for a done function that says it is always done
static inline bool pbio_control_always_done(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return true;
}

typedef struct _pbio_control_t {
    pbio_control_settings_t settings;
    pbio_actuation_t after_stop;
    pbio_trajectory_t trajectory;
    pbio_rate_integrator_t rate_integrator;
    pbio_count_integrator_t count_integrator;
    pbio_control_done_t is_done_func;
    bool stalled;
} pbio_control_t;

pbio_error_t pbio_control_get_ratio_settings(pbio_control_t *ctl, char *gear_ratio_str, char *counts_per_degree_str);

pbio_error_t pbio_control_get_limits(pbio_control_t *ctl,
                                     int32_t *max_speed,
                                     int32_t *acceleration);

pbio_error_t pbio_control_set_limits(pbio_control_t *ctl,
                                     int32_t max_speed,
                                     int32_t acceleration);

pbio_error_t pbio_control_get_pid_settings(pbio_control_t *ctl,
                                           int16_t *pid_kp,
                                           int16_t *pid_ki,
                                           int16_t *pid_kd,
                                           int32_t *tight_loop_time,
                                           int32_t *position_tolerance,
                                           int32_t *speed_tolerance,
                                           int32_t *stall_speed_limit,
                                           int32_t *stall_time);

pbio_error_t pbio_control_set_pid_settings(pbio_control_t *ctl,
                                           int16_t pid_kp,
                                           int16_t pid_ki,
                                           int16_t pid_kd,
                                           int32_t tight_loop_time,
                                           int32_t position_tolerance,
                                           int32_t speed_tolerance,
                                           int32_t stall_speed_limit,
                                           int32_t stall_time);

void control_update_angle_target(pbio_control_t *ctl, int32_t time_now, int32_t count_now, int32_t rate_now, pbio_actuation_t *actuation_type, int32_t *control);
void control_update_time_target(pbio_control_t *ctl, int32_t time_now, int32_t count_now, int32_t rate_now, pbio_actuation_t *actuation_type, int32_t *control);

#endif // _PBIO_CONTROL_H_
