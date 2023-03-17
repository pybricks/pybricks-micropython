// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <stdlib.h>

#include <pbio/config.h>
#include <pbio/control_settings.h>
#include <pbio/int_math.h>
#include <pbio/observer.h>

/**
 * Converts milliseconds to time ticks used by controller.
 *
 * @param [in] ms             Time in milliseconds.
 * @return                    Time converted to control ticks.
 */
uint32_t pbio_control_time_ms_to_ticks(uint32_t ms) {
    if (ms > UINT32_MAX / PBIO_TRAJECTORY_TICKS_PER_MS) {
        return UINT32_MAX;
    }
    return ms * PBIO_TRAJECTORY_TICKS_PER_MS;
}

/**
 * Converts time ticks used by controller to milliseconds.
 *
 * @param [in] ticks          Control timer ticks.
 * @return                    Time converted to milliseconds.
 */
uint32_t pbio_control_time_ticks_to_ms(uint32_t ticks) {
    return ticks / PBIO_TRAJECTORY_TICKS_PER_MS;
}

/**
 * Converts position-like control units to application-specific units.
 *
 * This should only be used if input/ouput are within known bounds.
 *
 * @param [in] s              Control settings containing the scale.
 * @param [in] input          Signal in control units.
 * @return                    Signal in application units.
 */
int32_t pbio_control_settings_ctl_to_app(const pbio_control_settings_t *s, int32_t input) {
    return input / s->ctl_steps_per_app_step;
}

/**
 * Converts position-like control units to application-specific units.
 *
 * This can be used with large inputs but there is more overhead.
 *
 * @param [in] s              Control settings containing the scale.
 * @param [in] input          Signal in control units.
 * @return                    Signal in application units.
 */
int32_t pbio_control_settings_ctl_to_app_long(const pbio_control_settings_t *s, const pbio_angle_t *input) {
    return pbio_angle_to_low_res(input, s->ctl_steps_per_app_step);
}

/**
 * Converts application-specific units to position-like control units.
 *
 * @param [in] s              Control settings containing the scale.
 * @param [in] input          Signal in application units.
 * @return                    Signal in control units.
 */
int32_t pbio_control_settings_app_to_ctl(const pbio_control_settings_t *s, int32_t input) {
    if (input > INT32_MAX / s->ctl_steps_per_app_step) {
        return INT32_MAX;
    }
    if (input < -INT32_MAX / s->ctl_steps_per_app_step) {
        return -INT32_MAX;
    }
    return input * s->ctl_steps_per_app_step;
}

/**
 * Converts application-specific units to position-like control units.
 *
 * This can be used with large inputs but there is more overhead.
 *
 * @param [in]  s              Control settings containing the scale.
 * @param [in]  input          Signal in application units.
 * @param [out] output         Signal in control units.
 */
void pbio_control_settings_app_to_ctl_long(const pbio_control_settings_t *s, int32_t input, pbio_angle_t *output) {
    pbio_angle_from_low_res(output, input, s->ctl_steps_per_app_step);
}

/**
 * Converts actuation-like control units to application-specific units.
 *
 * @param [in] input          Actuation in control units (uNm).
 * @return                    Actuation in application units (mNm).
 */
int32_t pbio_control_settings_actuation_ctl_to_app(int32_t input) {
    // All applications currently use this scale, but it could be generalized
    // to a application specific conversion constant.
    return input / 1000;
}

/**
 * Converts application-specific units to actuation-like control units.
 *
 * @param [in] input          Actuation in application units (mNm).
 * @return                    Actuation in control units (uNm).
 */
int32_t pbio_control_settings_actuation_app_to_ctl(int32_t input) {
    // All applications currently use this scale, but it could be generalized
    // to a application specific conversion constant.
    return input * 1000;
}

/**
 * Multiplies an angle (mdeg), speed (mdeg/s) or angle integral (mdeg s)
 * by a control gain and scales to uNm.
 *
 * @param [in] value         Input value (mdeg, mdeg/s, or mdeg s).
 * @param [in] gain          Gain in uNm/deg, uNm/(deg/s), or uNm/(deg s).
 * @return                   Torque in uNm.
 */
int32_t pbio_control_settings_mul_by_gain(int32_t value, int32_t gain) {
    return pbio_int_math_mult_then_div(gain, value, 1000);
}

/**
 * Divides a torque (uNm) by a control gain to get an angle (mdeg),
 * speed (mdeg/s) or angle integral (mdeg s), and accounts for scaling.
 *
 * Only positive gains are allowed. If the gain is zero or less, this returns
 * zero.
 *
 * @param [in] value         Input value (uNm).
 * @param [in] gain          Positive gain in uNm/deg, uNm/(deg/s), or uNm/(deg s).
 * @return                   Result in mdeg, mdeg/s, or mdeg s.
 */
int32_t pbio_control_settings_div_by_gain(int32_t value, int32_t gain) {
    if (gain < 1) {
        return 0;
    }
    return pbio_int_math_mult_then_div(value, 1000, gain);
}

/**
 * Multiplies a value by the loop time in seconds.
 *
 * @param [in] input          Input value.
 * @return                    Input scaled by loop time in seconds.
 */
int32_t pbio_control_settings_mul_by_loop_time(int32_t input) {
    return input / (1000 / PBIO_CONFIG_CONTROL_LOOP_TIME_MS);
}

/**
 * Checks if a time sample is equal to or newer than a given base time stamp.
 *
 * @param [in] sample         Sample time.
 * @param [in] base           Base time to compare to.
 * @return                    True if sample time is equal to or newer than base time, else false.
 */
bool pbio_control_settings_time_is_later(uint32_t sample, uint32_t base) {
    return sample - base < UINT32_MAX / 2;
}

/**
 * Gets the control limits for movement and actuation, in application units.
 *
 * @param [in]  s             Control settings structure from which to read.
 * @param [out] speed         Speed limit in application units.
 * @param [out] acceleration  Absolute rate of change of the speed during on-ramp of the maneuver.
 * @param [out] deceleration  Absolute rate of change of the speed during off-ramp of the maneuver.
 */
void pbio_control_settings_get_trajectory_limits(const pbio_control_settings_t *s, int32_t *speed, int32_t *acceleration, int32_t *deceleration) {
    *speed = pbio_control_settings_ctl_to_app(s, s->speed_max);
    *acceleration = pbio_control_settings_ctl_to_app(s, s->acceleration);
    *deceleration = pbio_control_settings_ctl_to_app(s, s->deceleration);
}

/**
 * Sets the control limits for movement in application units.
 *
 * @param [in] s              Control settings structure from which to read.
 * @param [in] speed          Speed limit in application units.
 * @param [in] acceleration   Absolute rate of change of the speed during on-ramp of the maneuver.
 * @param [in] deceleration   Absolute rate of change of the speed during off-ramp of the maneuver.
 * @return                    ::PBIO_SUCCESS on success
 *                            ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_trajectory_limits(pbio_control_settings_t *s, int32_t speed, int32_t acceleration, int32_t deceleration) {
    // Validate that all inputs are within allowed bounds.
    pbio_error_t err = pbio_trajectory_validate_speed_limit(s->ctl_steps_per_app_step, speed);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_trajectory_validate_acceleration_limit(s->ctl_steps_per_app_step, acceleration);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_trajectory_validate_acceleration_limit(s->ctl_steps_per_app_step, deceleration);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    s->speed_max = pbio_control_settings_app_to_ctl(s, speed);
    s->acceleration = pbio_control_settings_app_to_ctl(s, acceleration);
    s->deceleration = pbio_control_settings_app_to_ctl(s, deceleration);
    return PBIO_SUCCESS;
}

/**
 * Gets the control limits for actuation, in application units.
 *
 * @param [in]  s             Control settings structure from which to read.
 * @return      actuation     Upper limit on actuation.
 */
int32_t pbio_control_settings_get_actuation_limit(const pbio_control_settings_t *s) {
    return pbio_control_settings_actuation_ctl_to_app(s->actuation_max);
}

/**
 * Sets the control limit for actuation, in application units.
 *
 * @param [in] s              Control settings structure from which to read.
 * @param [in] limit          Upper limit on actuation.
 * @return                    ::PBIO_SUCCESS on success
 *                            ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_actuation_limit(pbio_control_settings_t *s, int32_t limit) {
    if (limit < 1 || limit > pbio_control_settings_actuation_ctl_to_app(pbio_observer_get_max_torque())) {
        return PBIO_ERROR_INVALID_ARG;
    }
    s->actuation_max = pbio_control_settings_actuation_app_to_ctl(limit);
    return PBIO_SUCCESS;
}

/**
 * Gets the PID control parameters.
 *
 * Kp, Ki, and Kd are returned given in control units. Everything else in application units.
 *
 * @param [in]  s                    Control settings structure from which to read.
 * @param [out] pid_kp               Position error feedback constant.
 * @param [out] pid_ki               Accumulated error feedback constant.
 * @param [out] pid_kd               Speed error feedback constant.
 * @param [out] integral_deadzone    Zone (angle) around the target within which the integrator should not accumulate errors.
 * @param [out] integral_change_max  Absolute bound on the rate at which the integrator accumulates errors, in application units.
 */
void pbio_control_settings_get_pid(const pbio_control_settings_t *s, int32_t *pid_kp, int32_t *pid_ki, int32_t *pid_kd, int32_t *integral_deadzone, int32_t *integral_change_max) {
    *pid_kp = s->pid_kp;
    *pid_ki = s->pid_ki;
    *pid_kd = s->pid_kd;
    *integral_deadzone = pbio_control_settings_ctl_to_app(s, s->integral_deadzone);
    *integral_change_max = pbio_control_settings_ctl_to_app(s, s->integral_change_max);
}

/**
 * Verifies that a position-like limit (e.g. angle tolerance) has a valid value.
 *
 * @param [in] s                    Control settings structure.
 * @param [in] value                Value to be tested.
 * @return                          ::PBIO_SUCCESS or ::PBIO_ERROR_INVALID_ARG.
 */
static pbio_error_t pbio_control_settings_validate_position_setting(pbio_control_settings_t *s, int32_t value) {
    if (value < 0 || value > pbio_control_settings_ctl_to_app(s, INT32_MAX)) {
        return PBIO_ERROR_INVALID_ARG;
    }
    return PBIO_SUCCESS;
}

/**
 * Sets the PID control parameters.
 *
 * Kp, Ki, and Kd should be given in control units. Everything else in application units.
 *
 * @param [in] s                    Control settings structure to write to.
 * @param [in] pid_kp               Position error feedback constant.
 * @param [in] pid_ki               Accumulated error feedback constant.
 * @param [in] pid_kd               Speed error feedback constant.
 * @param [in] integral_deadzone    Zone (angle) around the target within which the integrator should not accumulate errors.
 * @param [in] integral_change_max  Absolute bound on the rate at which the integrator accumulates errors, in application units.
 * @return                           ::PBIO_SUCCESS on success
 *                                   ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_pid(pbio_control_settings_t *s, int32_t pid_kp, int32_t pid_ki, int32_t pid_kd, int32_t integral_deadzone, int32_t integral_change_max) {
    if (pid_kp < 0 || pid_ki < 0 || pid_kd < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // integral_change_max has physical units of speed, so must satisfy bound.
    pbio_error_t err = pbio_trajectory_validate_speed_limit(s->ctl_steps_per_app_step, integral_change_max);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_settings_validate_position_setting(s, integral_deadzone);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    s->pid_kp = pid_kp;
    s->pid_ki = pid_ki;
    s->pid_kd = pid_kd;
    s->integral_deadzone = pbio_control_settings_app_to_ctl(s, integral_deadzone);
    s->integral_change_max = pbio_control_settings_app_to_ctl(s, integral_change_max);
    return PBIO_SUCCESS;
}

/**
 * Gets the tolerances associated with reaching a position target.
 * @param [in]  s           Control settings structure from which to read.
 * @param [out] speed       Speed tolerance in application units.
 * @param [out] position    Position tolerance in application units.
 */
void pbio_control_settings_get_target_tolerances(const pbio_control_settings_t *s, int32_t *speed, int32_t *position) {
    *position = pbio_control_settings_ctl_to_app(s, s->position_tolerance);
    *speed = pbio_control_settings_ctl_to_app(s, s->speed_tolerance);
}

/**
 * Sets the tolerances associated with reaching a position target, in application units.
 *
 * @param [in] s            Control settings structure to write to.
 * @param [in] speed        Speed tolerance in application units.
 * @param [in] position     Position tolerance in application units.
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_target_tolerances(pbio_control_settings_t *s, int32_t speed, int32_t position) {
    if (position < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbio_error_t err = pbio_trajectory_validate_speed_limit(s->ctl_steps_per_app_step, speed);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_settings_validate_position_setting(s, position);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    s->position_tolerance = pbio_control_settings_app_to_ctl(s, position);
    s->speed_tolerance = pbio_control_settings_app_to_ctl(s, speed);
    return PBIO_SUCCESS;
}

/**
 * Gets the tolerances associated with the controller being stalled, in application units.
 *
 * @param [in]  s           Control settings structure from which to read.
 * @param [out] speed       If this speed can't be reached with maximum actuation, it is stalled.
 * @param [out] time        Minimum consecutive stall time (ticks) before stall flag getter returns true.
 */
void pbio_control_settings_get_stall_tolerances(const pbio_control_settings_t *s, int32_t *speed, uint32_t *time) {
    *speed = pbio_control_settings_ctl_to_app(s, s->stall_speed_limit);
    *time = pbio_control_time_ticks_to_ms(s->stall_time);
}

/**
 * Sets the tolerances associated with the controller being stalled, in application units.
 *
 * @param [in] s            Control settings structure to write to.
 * @param [in] speed        If this speed can't be reached with maximum actuation, it is stalled.
 * @param [in] time         Minimum consecutive stall time (ticks) before stall flag getter returns true.
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_stall_tolerances(pbio_control_settings_t *s, int32_t speed, uint32_t time) {
    pbio_error_t err = pbio_trajectory_validate_speed_limit(s->ctl_steps_per_app_step, speed);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    s->stall_speed_limit = pbio_control_settings_app_to_ctl(s, speed);
    s->stall_time = pbio_control_time_ms_to_ticks(time);
    return PBIO_SUCCESS;
}
