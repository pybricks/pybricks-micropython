// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdlib.h>

#include <pbdrv/clock.h>

#include <pbio/config.h>
#include <pbio/control.h>
#include <pbio/math.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>


static bool pbio_control_check_completion(pbio_control_t *ctl, int32_t time, int32_t count, int32_t rate) {

    // If no control is active, then all targets are complete.
    if (!pbio_control_is_active(ctl)) {
        return true;
    }

    // Timed maneuvers are done when the full duration has passed.
    if (pbio_control_type_is_time(ctl)) {
        return time - ctl->trajectory.t3 >= 0;
    }

    // What remains now is to deal with angle-based maneuvers. As with time
    // based trajectories, we want at least the duration to pass.
    if (time - ctl->trajectory.t3 < 0) {
        return false;
    }

    // For a nonzero final speed, we're done once we're at or past
    // the target, no matter the tolerances. Equivalently, we're done
    // once the sign of the angle error differs from the speed sign.
    if (ctl->trajectory.w3 != 0) {
        return pbio_math_sign(ctl->trajectory.th3 - count) != pbio_math_sign(ctl->trajectory.w3);
    }

    // For zero final speed, we need to at least stand still, so return false
    // when we're still moving faster than the tolerance.
    if (abs(rate) > ctl->settings.rate_tolerance) {
        return false;
    }

    // Once we stand still, we're complete if the distance to the
    // target is equal to or less than the allowed tolerance.
    return abs(ctl->trajectory.th3 - count) <= ctl->settings.count_tolerance;
}

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
    torque = pbio_math_clamp(torque, ctl->settings.max_torque);

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
    ctl->on_target = pbio_control_check_completion(ctl, time_ref, state->count, state->rate);

    // Save (low-pass filtered) load for diagnostics
    ctl->load = (ctl->load * (100 - PBIO_CONTROL_LOOP_TIME_MS) + torque * PBIO_CONTROL_LOOP_TIME_MS) / 100;

    // Decide actuation based on whether control is on target.
    if (!ctl->on_target) {
        // If we're not on target yet, we keep actuating with
        // the PID torque value that has just been calculated.
        *actuation = PBIO_ACTUATION_TORQUE;
        *control = torque;
    } else {
        // If on target, decide what to do next using the on-completion type.
        switch (ctl->on_completion) {
            case PBIO_CONTROL_ON_COMPLETION_COAST:
                *actuation = PBIO_ACTUATION_COAST;
                *control = 0;
                pbio_control_stop(ctl);
                break;
            case PBIO_CONTROL_ON_COMPLETION_BRAKE:
                *actuation = PBIO_ACTUATION_BRAKE;
                *control = 0;
                pbio_control_stop(ctl);
                break;
            case PBIO_CONTROL_ON_COMPLETION_CONTINUE:
            // Fall through, same as hold.
            case PBIO_CONTROL_ON_COMPLETION_HOLD:
                // Holding position or continuing the trajectory just means we
                // have to keep actuating with the PID torque value that has
                // just been calculated.
                *actuation = PBIO_ACTUATION_TORQUE;
                *control = torque;

                // If we are getting here on completion of a timed command with
                // a stationary endpoint, convert it to a stationary angle
                // based command and hold it.
                if (pbio_control_type_is_time(ctl) && ctl->trajectory.w3 == 0) {
                    pbio_control_start_hold_control(ctl, time_now, state->count);
                }
                break;
        }
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
    ctl->stalled = false;
}

pbio_error_t pbio_control_start_angle_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t target_count, int32_t target_rate, pbio_control_on_completion_t on_completion) {

    pbio_error_t err;

    // Set new maneuver action and stop type, and state
    ctl->on_completion = on_completion;
    ctl->on_target = false;

    // Common trajectory parameters for all cases covered here.
    pbio_trajectory_command_t command = {
        .th3 = target_count,
        .wt = target_rate,
        .wmax = ctl->settings.rate_max,
        .a0_abs = ctl->settings.acceleration,
        .a2_abs = ctl->settings.deceleration,
        .continue_running = on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    };

    // Given the control status, fill in remaining commands and get trajectory.
    if (!pbio_control_is_active(ctl)) {
        // If no control is ongoing, we just start from the measured state.
        command.t0 = time_now;
        command.th0 = state->count;
        command.w0 = state->rate;

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // Otherwise, If control is active, (re)start from the current
        // reference. This way the reference just branches off on a new
        // trajectory instead of falling back slightly, avoiding a speed drop.
        command.t0 = pbio_control_get_ref_time(ctl, time_now);
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&ctl->trajectory, command.t0, &ref);
        command.th0 = ref.count;
        command.w0 = ref.rate;

        // Before we override the trajectory to renew it, get the starting
        // point of the current speed/angle segment of the reference. We may
        // need it below.
        int32_t time_vertex;
        pbio_trajectory_reference_t ref_vertex;
        pbio_trajectory_get_last_vertex(&ctl->trajectory, command.t0, &time_vertex, &ref_vertex);

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // If the new trajectory is tangent to the current one, we can do
        // better than just branching off. Instead, we can adjust the command
        // so it starts from the same point as the previous trajectory. This
        // avoids rounding errors when restarting commands in a tight loop.
        if (ctl->trajectory.a0 == ref.acceleration) {

            // Update command with shifted starting point, equal to ongoing
            // maneuver.
            command.t0 = time_vertex;
            command.th0 = ref_vertex.count;
            command.w0 = ref_vertex.rate;

            // Recalculate the trajectory from the shifted starting point.
            err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
            if (err != PBIO_SUCCESS) {
                return err;
            }
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

pbio_error_t pbio_control_start_relative_angle_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t relative_target_count, int32_t target_rate, pbio_control_on_completion_t on_completion) {

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

    return pbio_control_start_angle_control(ctl, time_now, state, target_count, target_rate, on_completion);
}

pbio_error_t pbio_control_start_hold_control(pbio_control_t *ctl, int32_t time_now, int32_t target_count) {

    // Set new maneuver action and stop type, and state
    ctl->on_completion = PBIO_CONTROL_ON_COMPLETION_HOLD;
    ctl->on_target = false;

    // Compute new maneuver based on user argument, starting from the initial state
    pbio_trajectory_command_t command = {
        .t0 = pbio_control_get_ref_time(ctl, time_now),
        .th0 = target_count,
        .th0_ext = 0,
        .wt = 0,
        .continue_running = false,
    };
    pbio_trajectory_make_constant(&ctl->trajectory, &command);
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


pbio_error_t pbio_control_start_timed_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t duration, int32_t target_rate, pbio_control_on_completion_t on_completion) {

    pbio_error_t err;

    // Set new maneuver action and stop type, and state
    ctl->on_completion = on_completion;
    ctl->on_target = false;

    // Common trajectory parameters for the cases covered here.
    pbio_trajectory_command_t command = {
        .t0 = time_now,
        .duration = duration,
        .wt = target_rate,
        .wmax = ctl->settings.rate_max,
        .a0_abs = ctl->settings.acceleration,
        .a2_abs = ctl->settings.deceleration,
        .continue_running = on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    };

    // Given the control status, fill in remaining commands and get trajectory.
    if (!pbio_control_is_active(ctl)) {
        // If no control is ongoing, we just start from the measured state.
        command.t0 = time_now;
        command.th0 = state->count;
        command.w0 = state->rate;

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_time_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // Otherwise, If control is active, (re)start from the current
        // reference. This way the reference just branches off on a new
        // trajectory instead of falling back slightly, avoiding a speed drop.
        int32_t time_ref = pbio_control_get_ref_time(ctl, time_now);
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&ctl->trajectory, time_ref, &ref);
        command.th0 = ref.count;
        command.th0_ext = ref.count_ext;
        command.w0 = ref.rate;

        // Before we override the trajectory to renew it, get the starting
        // point of the current speed/angle segment of the reference. We may
        // need it below.
        int32_t time_vertex;
        pbio_trajectory_reference_t ref_vertex;
        pbio_trajectory_get_last_vertex(&ctl->trajectory, command.t0, &time_vertex, &ref_vertex);

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_time_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // If the new trajectory is tangent to the current one, we can do
        // better than just branching off. Instead, we can adjust the command
        // so it starts from the same point as the previous trajectory. This
        // avoids rounding errors when restarting commands in a tight loop.
        if (pbio_control_type_is_time(ctl) && ctl->trajectory.a0 == ref.acceleration) {

            // Update command with shifted starting point, equal to ongoing
            // maneuver.
            command.t0 = time_vertex;
            command.th0 = ref_vertex.count;
            command.th0_ext = ref_vertex.count_ext;
            command.w0 = ref_vertex.rate;

            // We shifted the start time into the past, so we must adjust
            // duration accordingly.
            command.duration += (time_now - time_vertex);

            // Recalculate the trajectory from the shifted starting point.
            err = pbio_trajectory_new_time_command(&ctl->trajectory, &command);
            if (err != PBIO_SUCCESS) {
                return err;
            }
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

int32_t pbio_control_counts_to_user(pbio_control_settings_t *s, int32_t counts) {
    return pbio_math_div_i32_fix16(counts, s->counts_per_unit);
}

int32_t pbio_control_user_to_counts(pbio_control_settings_t *s, int32_t user) {
    return pbio_math_mul_i32_fix16(user, s->counts_per_unit);
}

void pbio_control_settings_get_limits(pbio_control_settings_t *s, int32_t *speed, int32_t *acceleration, int32_t *deceleration, int32_t *torque) {
    *speed = pbio_control_counts_to_user(s, s->rate_max);
    *acceleration = pbio_control_counts_to_user(s, s->acceleration);
    *deceleration = pbio_control_counts_to_user(s, s->deceleration);
    *torque = s->max_torque / 1000;
}

pbio_error_t pbio_control_settings_set_limits(pbio_control_settings_t *s, int32_t speed, int32_t acceleration, int32_t deceleration, int32_t torque) {
    if (speed < 1 || acceleration < 1 || deceleration < 1 || torque < 1) {
        return PBIO_ERROR_INVALID_ARG;
    }
    s->rate_max = pbio_control_user_to_counts(s, speed);
    s->acceleration = pbio_control_user_to_counts(s, acceleration);
    s->deceleration = pbio_control_user_to_counts(s, deceleration);
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
    // Angle controllers may pause the time so the reference position does not
    // keep accumulating while the controller is stuck.
    if (pbio_control_type_is_angle(ctl)) {
        return pbio_count_integrator_get_ref_time(&ctl->count_integrator, time_now);
    }
    // In all other cases, it is just the current time.
    return time_now;
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

bool pbio_control_is_stalled(pbio_control_t *ctl, int32_t *stall_duration) {

    // Return false if no control is active or if we're not stalled.
    if (!pbio_control_is_active(ctl) || !ctl->stalled) {
        *stall_duration = 0;
        return false;
    }

    // Get time since stalling began.
    int32_t time_pause_begin = ctl->type == PBIO_CONTROL_ANGLE ? ctl->count_integrator.time_pause_begin : ctl->rate_integrator.time_pause_begin;
    *stall_duration = (pbdrv_clock_get_us() - time_pause_begin) / US_PER_MS;

    return true;
}

bool pbio_control_is_done(pbio_control_t *ctl) {
    return ctl->type == PBIO_CONTROL_NONE || ctl->on_target;
}

int32_t pbio_control_get_load(pbio_control_t *ctl) {
    return ctl->type == PBIO_CONTROL_NONE ? 0 : ctl->load;
}
