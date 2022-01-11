// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdlib.h>

#include <pbio/config.h>
#include <pbio/control.h>
#include <pbio/math.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>

void pbio_control_update(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, pbio_trajectory_reference_t *ref, pbio_actuation_t *actuation, int32_t *control) {

    // Declare current time, positions, rates, and their reference value and error
    int32_t time_ref;
    int32_t count_err, count_err_integral, rate_err_integral;
    int32_t rate_err, rate_feedback;
    int32_t torque, torque_due_to_proportional, torque_due_to_integral, torque_due_to_derivative;

    // Get the time at which we want to evaluate the reference position/velocities.
    // This compensates for any time we may have spent pausing when the motor was stalled.
    time_ref = pbio_control_get_ref_time(ctl, time_now);

    // Get reference signals
    pbio_trajectory_get_reference(&ctl->trajectory, time_ref, ref);

    // Select either the estimated speed or the reported/measured speed for use in feedback.
    rate_feedback = ctl->settings.use_estimated_rate ? state->rate_est : state->rate;

    // Calculate control errors, depending on whether we do angle control or speed control
    if (pbio_control_type_is_angle(ctl)) {

        // Specify in which region integral control should be active. This is
        // at least the error that would still lead to maximum  proportional
        // control, with a factor of 2 so we begin integrating a bit sooner.
        int32_t integral_range = (ctl->settings.max_torque / ctl->settings.pid_kp) * 2;

        // Update count integral error and get current error state
        pbio_count_integrator_update(&ctl->count_integrator, time_now, state->count, ref->count, ctl->trajectory.th3, integral_range, ctl->settings.integral_rate);
        pbio_count_integrator_get_errors(&ctl->count_integrator, state->count, ref->count, &count_err, &count_err_integral);
        rate_err = ref->rate - rate_feedback;
    } else {
        // For time/speed based commands, the main error is speed. It integrates into a quantity with unit of position.
        // There is no count integral control, because we do not need a second order integrator for speed control.
        pbio_rate_integrator_get_errors(&ctl->rate_integrator, rate_feedback, ref->rate, state->count, ref->count, &rate_err, &rate_err_integral);
        count_err = rate_err_integral;
        count_err_integral = 0;
    }

    // Corresponding PID control signal
    torque_due_to_proportional = ctl->settings.pid_kp * count_err;
    torque_due_to_derivative = ctl->settings.pid_kd * rate_err;
    torque_due_to_integral = (ctl->settings.pid_ki * (count_err_integral / US_PER_MS)) / MS_PER_SECOND;

    // Total torque signal, capped by the actuation limit
    torque = torque_due_to_proportional + torque_due_to_integral + torque_due_to_derivative;
    torque = max(-ctl->settings.max_torque, min(torque, ctl->settings.max_torque));

    // This completes the computation of the control signal.
    // The next steps take care of handling windup, or triggering a stop if we are on target.

    // We want to stop building up further errors if we are at the proportional torque limit. So, we pause the trajectory
    // if we get at this limit. We wait a little longer though, to make sure it does not fall back to below the limit
    // within one sample, which we can predict using the current rate times the loop time, with a factor two tolerance.
    int32_t max_windup_torque = ctl->settings.max_torque + (ctl->settings.pid_kp * abs(state->rate) * PBIO_CONTROL_LOOP_TIME_MS * 2) / MS_PER_SECOND;

    // Position anti-windup: pause trajectory or integration if falling behind despite using maximum torque
    bool pause_integration =
        // Pause if proportional torque is beyond maximum windup torque:
        abs(torque_due_to_proportional) >= max_windup_torque &&
        // But not if we're trying to run in the other direction (else we can get unstuck by just reversing).
        pbio_math_sign(torque_due_to_proportional) != -pbio_math_sign(rate_err) &&
        // But not if we should be accelerating in the other direction (else we can get unstuck by just reversing).
        pbio_math_sign(torque_due_to_proportional) != -pbio_math_sign(ref->acceleration);

    // Position anti-windup in case of angle control (accumulated position error may not get too high)
    if (pbio_control_type_is_angle(ctl)) {
        if (pause_integration) {
            // We are at the torque limit and we should prevent further position error integration.
            pbio_count_integrator_pause(&ctl->count_integrator, time_now, state->count, ref->count);
        } else {
            // Not at the limit so continue integrating errors
            pbio_count_integrator_resume(&ctl->count_integrator, time_now, state->count, ref->count);
        }
    }
    // Position anti-windup in case of timed speed control (speed integral may not get too high)
    else {
        if (pause_integration) {
            // We are at the torque limit and we should prevent further speed error integration.
            pbio_rate_integrator_pause(&ctl->rate_integrator, time_now, state->count, ref->count);
        } else {
            // Not at the limit so continue integrating errors
            pbio_rate_integrator_resume(&ctl->rate_integrator, time_now, state->count, ref->count);
        }
    }

    // Check if controller is stalled
    ctl->stalled = pbio_control_type_is_angle(ctl) ?
        pbio_count_integrator_stalled(&ctl->count_integrator, time_now, state->rate, ctl->settings.stall_time, ctl->settings.stall_rate_limit) :
        pbio_rate_integrator_stalled(&ctl->rate_integrator, time_now, state->rate, ctl->settings.stall_time, ctl->settings.stall_rate_limit);

    // Check if we are on target
    ctl->on_target = ctl->on_target_func(&ctl->trajectory, &ctl->settings, time_ref, state->count, state->rate, ctl->stalled);

    // Save (low-pass filtered) load for diagnostics
    ctl->load = (ctl->load * (100 - PBIO_CONTROL_LOOP_TIME_MS) + torque * PBIO_CONTROL_LOOP_TIME_MS) / 100;

    // If we are done and the next action is passive then return zero actuation
    if (ctl->on_target && ctl->after_stop != PBIO_ACTUATION_HOLD) {
        *actuation = ctl->after_stop;
        *control = 0;
        pbio_control_stop(ctl);
    } else if (ctl->on_target && ctl->after_stop == PBIO_ACTUATION_HOLD && pbio_control_type_is_time(ctl)) {
        // If we are going to hold and we are already doing angle control, there is nothing we need to do.
        // But if we are going to hold when we are doing speed control right now, we must trigger a hold first.
        pbio_control_start_hold_control(ctl, time_now, state->count);

        // The new hold control does not take effect until the next iteration, so keep actuating for now.
        *actuation = PBIO_ACTUATION_TORQUE;
        *control = torque;
    } else {
        // The end point not reached, or we have to keep holding, so return the calculated torque for actuation
        *actuation = PBIO_ACTUATION_TORQUE;
        *control = torque;
    }

    // Log control data
    int32_t log_data[] = {
        time_ref - ctl->trajectory.t0,
        state->count,
        state->rate,
        *actuation,
        *control,
        ref->count,
        ref->rate,
        state->count_est,
        state->rate_est,
        torque_due_to_proportional,
        torque_due_to_integral,
        torque_due_to_derivative,
    };
    pbio_logger_update(&ctl->log, log_data);
}


void pbio_control_stop(pbio_control_t *ctl) {
    ctl->type = PBIO_CONTROL_NONE;
    ctl->on_target = true;
    ctl->on_target_func = pbio_control_on_target_always;
    ctl->stalled = false;
}

pbio_error_t pbio_control_start_angle_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t target_count, int32_t target_rate, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Set new maneuver action and stop type, and state
    ctl->after_stop = after_stop;
    ctl->on_target = false;
    ctl->on_target_func = pbio_control_on_target_angle;

    // Compute the trajectory
    if (!pbio_control_is_active(ctl)) {
        // If no control is ongoing, start from physical state
        err = pbio_trajectory_calc_time_new(&ctl->trajectory, time_now, state->count, target_count, state->rate, target_rate, ctl->settings.max_rate, ctl->settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // If control is ongoing, start from its current reference. First get time on current reference signal
        int32_t time_ref = pbio_control_get_ref_time(ctl, time_now);

        // Make the new trajectory and try to patch to existing one
        err = pbio_trajectory_calc_time_extend(&ctl->trajectory, time_ref, target_count, target_rate, ctl->settings.max_rate, ctl->settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Reset PID control if needed
    if (!pbio_control_type_is_angle(ctl)) {
        // New angle maneuver, so reset the rate integrator
        int32_t integrator_max = pbio_control_settings_get_max_integrator(&ctl->settings);
        pbio_count_integrator_reset(&ctl->count_integrator, ctl->trajectory.t0, ctl->trajectory.th0, ctl->trajectory.th0, integrator_max);

        // Reset load filter
        ctl->load = 0;
    }

    // Set the new control state
    ctl->type = PBIO_CONTROL_ANGLE;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_control_start_relative_angle_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t relative_target_count, int32_t target_rate, pbio_actuation_t after_stop) {

    // Get the count from which the relative count is to be counted
    int32_t count_start;

    // If no control is active, count from the physical count
    if (!pbio_control_is_active(ctl)) {
        count_start = state->count;
    }
    // Otherwise count from the current reference
    else {
        int32_t time_ref = pbio_control_get_ref_time(ctl, time_now);
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&ctl->trajectory, time_ref, &ref);
        count_start = ref.count;
    }

    // The target count is the start count plus the count to be traveled.  If speed is negative, traveled count also flips.
    int32_t target_count = count_start + (target_rate < 0 ? -relative_target_count : relative_target_count);

    // FIXME: Enable 0 angle and angle > 0 with excess speed as standard case instead to decelerate & return.
    if (target_count == count_start) {
        return pbio_control_start_hold_control(ctl, time_now, target_count);
    }

    return pbio_control_start_angle_control(ctl, time_now, state, target_count, target_rate, after_stop);
}

pbio_error_t pbio_control_start_hold_control(pbio_control_t *ctl, int32_t time_now, int32_t target_count) {

    // Set new maneuver action and stop type, and state
    ctl->after_stop = PBIO_ACTUATION_HOLD;
    ctl->on_target = false;
    ctl->on_target_func = pbio_control_on_target_always;

    // Compute new maneuver based on user argument, starting from the initial state
    pbio_trajectory_make_stationary(&ctl->trajectory, pbio_control_get_ref_time(ctl, time_now), target_count);
    // If called for the first time, set state and reset PID
    if (!pbio_control_type_is_angle(ctl)) {
        // Initialize or reset the PID control status for the given maneuver
        int32_t integrator_max = pbio_control_settings_get_max_integrator(&ctl->settings);
        pbio_count_integrator_reset(&ctl->count_integrator, time_now, ctl->trajectory.th0, ctl->trajectory.th0, integrator_max);

        // Reset load filter
        ctl->load = 0;
    }

    // This is an angular control maneuver
    ctl->type = PBIO_CONTROL_ANGLE;

    return PBIO_SUCCESS;
}


pbio_error_t pbio_control_start_timed_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t duration, int32_t target_rate, pbio_control_on_target_t stop_func, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Set new maneuver action and stop type, and state
    ctl->after_stop = after_stop;
    ctl->on_target = false;
    ctl->on_target_func = stop_func;

    // Compute the trajectory
    if (pbio_control_type_is_time(ctl)) {
        // If timed control is already ongoing make the new trajectory and try to patch to existing one
        err = pbio_trajectory_calc_angle_extend(&ctl->trajectory, time_now, duration, target_rate, ctl->settings.max_rate, ctl->settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else if (pbio_control_type_is_angle(ctl)) {
        // If position based control is ongoing, start from its current reference. First get current reference signal.
        int32_t time_ref = pbio_control_get_ref_time(ctl, time_now);
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&ctl->trajectory, time_ref, &ref);

        // Now start the timed trajectory from there
        err = pbio_trajectory_calc_angle_new(&ctl->trajectory, time_now, duration, ref.count, 0, ref.rate, target_rate, ctl->settings.max_rate, ctl->settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // If no control is ongoing, start from physical state
        err = pbio_trajectory_calc_angle_new(&ctl->trajectory, time_now, duration, state->count, 0, state->rate, target_rate, ctl->settings.max_rate, ctl->settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Reset PD control if needed
    if (!pbio_control_type_is_time(ctl)) {
        // New maneuver, so reset the rate integrator
        pbio_rate_integrator_reset(&ctl->rate_integrator, time_now, state->count, state->count);

        // Set the new control state
        ctl->type = PBIO_CONTROL_TIMED;

        // Reset load filter
        ctl->load = 0;
    }

    return PBIO_SUCCESS;
}

static bool _pbio_control_on_target_always(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return true;
}
pbio_control_on_target_t pbio_control_on_target_always = _pbio_control_on_target_always;

static bool _pbio_control_on_target_never(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return false;
}
pbio_control_on_target_t pbio_control_on_target_never = _pbio_control_on_target_never;

static bool _pbio_control_on_target_angle(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    // if not enough time has expired to be done even in the ideal case, we are certainly not done
    if (time - trajectory->t3 < 0) {
        return false;
    }

    // If distance to target is still bigger than the tolerance, we are not there yet.
    if (trajectory->th3 - count > settings->count_tolerance) {
        return false;
    }

    // // If distance past target is still bigger than the tolerance, we are too far, so not there yet
    if (count - trajectory->th3 > settings->count_tolerance) {
        return false;
    }

    // If the motor is not standing still, we are not there yet
    if (abs(rate) > settings->rate_tolerance) {
        return false;
    }

    // There's nothing left to do, so we must be on target
    return true;
}
pbio_control_on_target_t pbio_control_on_target_angle = _pbio_control_on_target_angle;

static bool _pbio_control_on_target_time(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return time >= trajectory->t3;
}
pbio_control_on_target_t pbio_control_on_target_time = _pbio_control_on_target_time;

static bool _pbio_control_on_target_stalled(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return stalled;
}
pbio_control_on_target_t pbio_control_on_target_stalled = _pbio_control_on_target_stalled;

int32_t pbio_control_counts_to_user(pbio_control_settings_t *s, int32_t counts) {
    return pbio_math_div_i32_fix16(counts, s->counts_per_unit);
}

int32_t pbio_control_user_to_counts(pbio_control_settings_t *s, int32_t user) {
    return pbio_math_mul_i32_fix16(user, s->counts_per_unit);
}

void pbio_control_settings_get_limits(pbio_control_settings_t *s, int32_t *speed, int32_t *acceleration, int32_t *torque) {
    *speed = pbio_control_counts_to_user(s, s->max_rate);
    *acceleration = pbio_control_counts_to_user(s, s->abs_acceleration);
    *torque = s->max_torque / 1000;
}

pbio_error_t pbio_control_settings_set_limits(pbio_control_settings_t *s, int32_t speed, int32_t acceleration, int32_t torque) {
    if (speed < 1 || acceleration < 1 || torque < 1) {
        return PBIO_ERROR_INVALID_ARG;
    }
    s->max_rate = pbio_control_user_to_counts(s, speed);
    s->abs_acceleration = pbio_control_user_to_counts(s, acceleration);
    s->max_torque = torque * 1000;
    return PBIO_SUCCESS;
}

void pbio_control_settings_get_pid(pbio_control_settings_t *s, int32_t *pid_kp, int32_t *pid_ki, int32_t *pid_kd, int32_t *integral_rate) {
    *pid_kp = s->pid_kp;
    *pid_ki = s->pid_ki;
    *pid_kd = s->pid_kd;
    *integral_rate = pbio_control_counts_to_user(s, s->integral_rate);
}

pbio_error_t pbio_control_settings_set_pid(pbio_control_settings_t *s, int32_t pid_kp, int32_t pid_ki, int32_t pid_kd, int32_t integral_rate) {
    if (pid_kp < 0 || pid_ki < 0 || pid_kd < 0 || integral_rate < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    s->pid_kp = pid_kp;
    s->pid_ki = pid_ki;
    s->pid_kd = pid_kd;
    s->integral_rate = pbio_control_user_to_counts(s, integral_rate);
    return PBIO_SUCCESS;
}

void pbio_control_settings_get_target_tolerances(pbio_control_settings_t *s, int32_t *speed, int32_t *position) {
    *position = pbio_control_counts_to_user(s, s->count_tolerance);
    *speed = pbio_control_counts_to_user(s, s->rate_tolerance);
}

pbio_error_t pbio_control_settings_set_target_tolerances(pbio_control_settings_t *s, int32_t speed, int32_t position) {
    if (position < 0 || speed < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    s->count_tolerance = pbio_control_user_to_counts(s, position);
    s->rate_tolerance = pbio_control_user_to_counts(s, speed);
    return PBIO_SUCCESS;
}

void pbio_control_settings_get_stall_tolerances(pbio_control_settings_t *s,  int32_t *speed, int32_t *time) {
    *speed = pbio_control_counts_to_user(s, s->stall_rate_limit);
    *time = s->stall_time / US_PER_MS;
}

pbio_error_t pbio_control_settings_set_stall_tolerances(pbio_control_settings_t *s, int32_t speed, int32_t time) {
    if (speed < 0 || time < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    s->stall_rate_limit = pbio_control_user_to_counts(s, speed);
    s->stall_time = time * US_PER_MS;
    return PBIO_SUCCESS;
}

int32_t pbio_control_settings_get_max_integrator(pbio_control_settings_t *s) {
    // If ki is very small, then the integrator is "unlimited"
    if (s->pid_ki <= 10) {
        return 1000000000;
    }
    // Get the maximum integrator value for which ki*integrator does not exceed max_torque
    return ((s->max_torque * US_PER_MS) / s->pid_ki) * MS_PER_SECOND;
}

int32_t pbio_control_get_ref_time(pbio_control_t *ctl, int32_t time_now) {

    if (pbio_control_type_is_angle(ctl)) {
        return pbio_count_integrator_get_ref_time(&ctl->count_integrator, time_now);
    }
    if (pbio_control_type_is_time(ctl)) {
        return time_now;
    }
    return 0;
}

bool pbio_control_is_active(pbio_control_t *ctl) {
    return ctl->type != PBIO_CONTROL_NONE;
}

bool pbio_control_type_is_angle(pbio_control_t *ctl) {
    return ctl->type == PBIO_CONTROL_ANGLE;
}

bool pbio_control_type_is_time(pbio_control_t *ctl) {
    return ctl->type == PBIO_CONTROL_TIMED;
}

bool pbio_control_is_stalled(pbio_control_t *ctl) {
    return ctl->type != PBIO_CONTROL_NONE && ctl->stalled;
}

bool pbio_control_is_done(pbio_control_t *ctl) {
    return ctl->type == PBIO_CONTROL_NONE || ctl->on_target;
}

int32_t pbio_control_get_load(pbio_control_t *ctl) {
    return ctl->type == PBIO_CONTROL_NONE ? 0 : ctl->load;
}
