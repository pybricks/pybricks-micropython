// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <stdlib.h>

#include <pbio/config.h>
#include <pbio/control_settings.h>

/**
 * Converts position-like control units to application-specific units.
 *
 * This should only be used if input/ouput are within known bounds.
 *
 * @param [in] s              Control settings containing the scale.
 * @param [in] input          Signal in control units.
 * @return                    Signal in application units.
 */
int32_t pbio_control_settings_ctl_to_app(pbio_control_settings_t *s, int32_t input) {
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
int32_t pbio_control_settings_ctl_to_app_long(pbio_control_settings_t *s, pbio_angle_t *input) {
    return pbio_angle_to_low_res(input, s->ctl_steps_per_app_step);
}

/**
 * Converts application-specific units to position-like control units.
 *
 * This should only be used if input/ouput are within known bounds.
 *
 * @param [in] s              Control settings containing the scale.
 * @param [in] input          Signal in application units.
 * @return                    Signal in control units.
 */
int32_t pbio_control_settings_app_to_ctl(pbio_control_settings_t *s, int32_t input) {
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
void pbio_control_settings_app_to_ctl_long(pbio_control_settings_t *s, int32_t input, pbio_angle_t *output) {
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
    // to a appplication specific conversion constant.
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
    // to a appplication specific conversion constant.
    return input * 1000;
}

/**
 * Gets the control limits for movement and actuation, in application units.
 *
 * @param [in]  s             Control settings structure from which to read.
 * @param [out] speed         Speed limit in application units.
 * @param [out] acceleration  Absolute rate of change of the speed during on-ramp of the maneuver.
 * @param [out] deceleration  Absolute rate of change of the speed during off-ramp of the maneuver.
 * @param [out] actuation     Upper limit on actuation.
 */
void pbio_control_settings_get_limits(pbio_control_settings_t *s, int32_t *speed, int32_t *acceleration, int32_t *deceleration, int32_t *actuation) {
    *speed = pbio_control_settings_ctl_to_app(s, s->speed_max);
    *acceleration = pbio_control_settings_ctl_to_app(s, s->acceleration);
    *deceleration = pbio_control_settings_ctl_to_app(s, s->deceleration);
    *actuation = pbio_control_settings_actuation_ctl_to_app(s->actuation_max);
}

/**
 * Sets the control limits for movement and actuation, in application units.
 *
 * @param [in] s              Control settings structure from which to read.
 * @param [in] speed          Speed limit in application units.
 * @param [in] acceleration   Absolute rate of change of the speed during on-ramp of the maneuver.
 * @param [in] deceleration   Absolute rate of change of the speed during off-ramp of the maneuver.
 * @param [in] actuation      Upper limit on actuation.
 * @return                    ::PBIO_SUCCESS on success
 *                            ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_limits(pbio_control_settings_t *s, int32_t speed, int32_t acceleration, int32_t deceleration, int32_t actuation) {
    if (speed < 1 || acceleration < 1 || deceleration < 1 || actuation < 1) {
        return PBIO_ERROR_INVALID_ARG;
    }
    s->speed_max = pbio_control_settings_app_to_ctl(s, speed);
    s->acceleration = pbio_control_settings_app_to_ctl(s, acceleration);
    s->deceleration = pbio_control_settings_app_to_ctl(s, deceleration);
    s->actuation_max = pbio_control_settings_actuation_app_to_ctl(actuation);
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
 * @param [out] integral_change_max  Absolute bound on the rate at which the integrator accumulates errors, in application units.
 */
void pbio_control_settings_get_pid(pbio_control_settings_t *s, int32_t *pid_kp, int32_t *pid_ki, int32_t *pid_kd, int32_t *integral_change_max) {
    *pid_kp = s->pid_kp;
    *pid_ki = s->pid_ki;
    *pid_kd = s->pid_kd;
    *integral_change_max = pbio_control_settings_ctl_to_app(s, s->integral_change_max);
}

/**
 * Sets the PID control parameters.
 *
 * Kp, Ki, and Kd should be given in control units. Everything else in application units.
 *
 * @param [in] s                     Control settings structure to write to.
 * @param [out] pid_kp               Position error feedback constant.
 * @param [out] pid_ki               Accumulated error feedback constant.
 * @param [out] pid_kd               Speed error feedback constant.
 * @param [out] integral_change_max  Absolute bound on the rate at which the integrator accumulates errors, in application units.
 * @return                           ::PBIO_SUCCESS on success
 *                                   ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_pid(pbio_control_settings_t *s, int32_t pid_kp, int32_t pid_ki, int32_t pid_kd, int32_t integral_change_max) {
    if (pid_kp < 0 || pid_ki < 0 || pid_kd < 0 || integral_change_max < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    s->pid_kp = pid_kp;
    s->pid_ki = pid_ki;
    s->pid_kd = pid_kd;
    s->integral_change_max = pbio_control_settings_app_to_ctl(s, integral_change_max);
    return PBIO_SUCCESS;
}

/**
 * Gets the tolerances associated with reaching a position target.
 * @param [in]  s           Control settings structure from which to read.
 * @param [out] speed       Speed tolerance in application units.
 * @param [out] position    Position tolerance in application units.
 */
void pbio_control_settings_get_target_tolerances(pbio_control_settings_t *s, int32_t *speed, int32_t *position) {
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
    if (position < 0 || speed < 0) {
        return PBIO_ERROR_INVALID_ARG;
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
 * @param [out] time        Minimum consequtive stall time (ticks) before stall flag getter returns true.
 */
void pbio_control_settings_get_stall_tolerances(pbio_control_settings_t *s, int32_t *speed, uint32_t *time) {
    *speed = pbio_control_settings_ctl_to_app(s, s->stall_speed_limit);
    *time = pbio_control_time_ticks_to_ms(s->stall_time);
}

/**
 * Sets the tolerances associated with the controller being stalled, in application units.
 *
 * @param [in] s            Control settings structure to write to.
 * @param [in] speed        If this speed can't be reached with maximum actuation, it is stalled.
 * @param [in] time         Minimum consequtive stall time (ticks) before stall flag getter returns true.
 * @return                  ::PBIO_SUCCESS on success
 *                          ::PBIO_ERROR_INVALID_ARG if any argument is negative.
 */
pbio_error_t pbio_control_settings_set_stall_tolerances(pbio_control_settings_t *s, int32_t speed, uint32_t time) {
    if (speed < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    s->stall_speed_limit = pbio_control_settings_app_to_ctl(s, speed);
    s->stall_time = pbio_control_time_ms_to_ticks(time);
    return PBIO_SUCCESS;
}
