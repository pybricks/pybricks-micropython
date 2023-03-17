// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#ifndef _PBIO_CONTROL_SETTINGS_H_
#define _PBIO_CONTROL_SETTINGS_H_

#include <stdint.h>

#include <pbio/angle.h>
#include <pbio/error.h>
#include <pbio/trajectory.h>

/**
 * @addtogroup ControlSettings pbio/control_settings: Control settings
 *
 * Provides getters, setters, and scalers for control settings.
 * @{
 */

/**
 * Control settings.
 */
typedef struct _pbio_control_settings_t {
    /**
     * The amount of "position-like control steps" for each "application step".
     *
     * This is used to scale practical end-user units (e.g. mm or deg) to
     * internal high resolution control units (e.g. mdeg).
     *
     * Each application (e.g. a servo with external gearing) defines its own
     * control units, application units, and the scaling factor to match.
     *
     * For example, in a servo with an external gear ratio of 5 with outputs in
     * degrees, and internal control in millidegrees, this value is 5000.
     *
     * All attributes given below are expressed in control units and control
     * ticks for time.
     */
    int32_t ctl_steps_per_app_step;
    /**
     * If this speed cannot be reached even with the maximum control signal
     * defined in actuation_max, the controller is stalled.
     */
    int32_t stall_speed_limit;
    /**
     * Minimum consecutive stall time before stall flag getter returns true.
     */
    uint32_t stall_time;
    /**
     * Speed that the user input will be capped to.
     */
    int32_t speed_max;
    /**
     * Speed used for calls without speed arguments.
     */
    int32_t speed_default;
    /**
     * Allowed speed deviation for controller to be on target. If the target
     * speed is zero such as at the end of the maneuver, this essentially sets
     * the speed threshold below which the controller is considered stationary.
     */
    int32_t speed_tolerance;
    /**
     * Allowed position deviation for controller to be on target.
     */
    int32_t position_tolerance;
    /**
     * Absolute rate of change of the speed during on-ramp of the maneuver.
     * This value will be used for the on-ramp even if the application
     * initially starts above the target speed, meaning it is decelerating.
     */
    int32_t acceleration;
    /**
     * Absolute rate of change of the speed during off-ramp of the maneuver.
     */
    int32_t deceleration;
    /**
     * Maximum feedback actuation value. On a motor this is the maximum torque.
     */
    int32_t actuation_max;
    /**
     * Position error feedback constant.
     */
    int32_t pid_kp;
    /**
     * Percentage of pid_kp to use for small errors at low speeds.
     */
    int32_t pid_kp_low_pct;
    /**
     * Threshold error angle below which to use lower kp constant at low speeds.
     */
    int32_t pid_kp_low_error_threshold;
    /**
     * Threshold speed below which to use the lower kp constant.
     */
    int32_t pid_kp_low_speed_threshold;
    /**
     * Accumulated position error feedback constant.
     */
    int32_t pid_ki;
    /**
     * Speed error feedback constant.
     */
    int32_t pid_kd;
    /**
     * Zone (angle) around the target within which the integral error should not grow.
     */
    int32_t integral_deadzone;
    /**
     * Absolute bound on the rate at which the integrator accumulates errors.
     */
    int32_t integral_change_max;
    /**
     * For how long to use hold mode before switching to passive mode in smart
     * brake and smart coast.
     */
    uint32_t smart_passive_hold_time;
} pbio_control_settings_t;

// Unit conversion functions:

uint32_t pbio_control_time_ms_to_ticks(uint32_t ms);
uint32_t pbio_control_time_ticks_to_ms(uint32_t ticks);
int32_t pbio_control_settings_ctl_to_app(const pbio_control_settings_t *s, int32_t input);
int32_t pbio_control_settings_ctl_to_app_long(const pbio_control_settings_t *s, const pbio_angle_t *input);
int32_t pbio_control_settings_app_to_ctl(const pbio_control_settings_t *s, int32_t input);
void pbio_control_settings_app_to_ctl_long(const pbio_control_settings_t *s, int32_t input, pbio_angle_t *output);
int32_t pbio_control_settings_actuation_ctl_to_app(int32_t input);
int32_t pbio_control_settings_actuation_app_to_ctl(int32_t input);
bool pbio_control_settings_time_is_later(uint32_t sample, uint32_t base);

// Scale values by given constants:

int32_t pbio_control_settings_mul_by_loop_time(int32_t input);
int32_t pbio_control_settings_mul_by_gain(int32_t value, int32_t gain);
int32_t pbio_control_settings_div_by_gain(int32_t value, int32_t gain);

// Control settings getters and setters:

void pbio_control_settings_get_trajectory_limits(const pbio_control_settings_t *s, int32_t *speed, int32_t *acceleration, int32_t *deceleration);
pbio_error_t pbio_control_settings_set_trajectory_limits(pbio_control_settings_t *s, int32_t speed, int32_t acceleration, int32_t deceleration);
int32_t pbio_control_settings_get_actuation_limit(const pbio_control_settings_t *s);
pbio_error_t pbio_control_settings_set_actuation_limit(pbio_control_settings_t *s, int32_t limit);
void pbio_control_settings_get_pid(const pbio_control_settings_t *s, int32_t *pid_kp, int32_t *pid_ki, int32_t *pid_kd, int32_t *integral_deadzone, int32_t *integral_change_max);
pbio_error_t pbio_control_settings_set_pid(pbio_control_settings_t *s, int32_t pid_kp, int32_t pid_ki, int32_t pid_kd, int32_t integral_deadzone, int32_t integral_change_max);
void pbio_control_settings_get_target_tolerances(const pbio_control_settings_t *s, int32_t *speed, int32_t *position);
pbio_error_t pbio_control_settings_set_target_tolerances(pbio_control_settings_t *s, int32_t speed, int32_t position);
void pbio_control_settings_get_stall_tolerances(const pbio_control_settings_t *s, int32_t *speed, uint32_t *time);
pbio_error_t pbio_control_settings_set_stall_tolerances(pbio_control_settings_t *s, int32_t speed, uint32_t time);

#endif // _PBIO_CONTROL_SETTINGS_H_

/** @} */
