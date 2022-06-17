// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdlib.h>

#include <pbdrv/clock.h>

#include <pbio/config.h>
#include <pbio/control.h>
#include <pbio/math.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>


static bool pbio_control_check_completion(pbio_control_t *ctl, int32_t time, pbio_control_state_t *state, pbio_trajectory_reference_t *end) {

    // If no control is active, then all targets are complete.
    if (!pbio_control_is_active(ctl)) {
        return true;
    }

    // Timed maneuvers are done when the full duration has passed.
    if (pbio_control_type_is_time(ctl)) {
        return time - end->time >= 0;
    }

    // What remains now is to deal with angle-based maneuvers. As with time
    // based trajectories, we want at least the duration to pass.
    if (time - end->time < 0) {
        return false;
    }

    // For a nonzero final speed, we're done once we're at or past
    // the target, no matter the tolerances. Equivalently, we're done
    // once the sign of the angle error differs from the speed sign.
    if (end->rate != 0) {
        return pbio_math_sign(end->count - state->count) != pbio_math_sign(end->rate);
    }

    // For zero final speed, we need to at least stand still, so return false
    // when we're still moving faster than the tolerance.
    if (abs(state->rate_est) > ctl->settings.rate_tolerance) {
        return false;
    }

    // Once we stand still, we're complete if the distance to the
    // target is equal to or less than the allowed tolerance.
    return abs(end->count - state->count) <= ctl->settings.count_tolerance;
}

void pbio_control_update(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, pbio_trajectory_reference_t *ref, pbio_dcmotor_actuation_t *actuation, int32_t *control) {

    // Declare current time, positions, rates, and their reference value and error
    int32_t count_err, count_err_integral, rate_err_integral;
    int32_t rate_err;
    int32_t torque, torque_due_to_proportional, torque_due_to_integral, torque_due_to_derivative;

    // Get reference signals at the reference time point in the trajectory.
    // This compensates for any time we may have spent pausing when the motor was stalled.
    pbio_trajectory_get_reference(&ctl->trajectory, pbio_control_get_ref_time(ctl, time_now), ref);

    // Get reference point we want to be at in the end, to check for completion.
    pbio_trajectory_reference_t ref_end;
    pbio_trajectory_get_endpoint(&ctl->trajectory, &ref_end);

    // Calculate control errors, depending on whether we do angle control or speed control
    if (pbio_control_type_is_angle(ctl)) {

        // Specify in which region integral control should be active. This is
        // at least the error that would still lead to maximum  proportional
        // control, with a factor of 2 so we begin integrating a bit sooner.
        int32_t integral_range = (ctl->settings.max_torque / ctl->settings.pid_kp) * 2;

        // Update count integral error and get current error state
        pbio_count_integrator_update(&ctl->count_integrator, time_now, state->count, ref->count, ref_end.count, integral_range, ctl->settings.integral_rate);
        pbio_count_integrator_get_errors(&ctl->count_integrator, state->count, ref->count, &count_err, &count_err_integral);
        rate_err = ref->rate - state->rate_est;
    } else {
        // For time/speed based commands, the main error is speed. It integrates into a quantity with unit of position.
        // There is no count integral control, because we do not need a second order integrator for speed control.
        pbio_rate_integrator_get_errors(&ctl->rate_integrator, state->rate_est, ref->rate, state->count, ref->count, &rate_err, &rate_err_integral);
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
    int32_t max_windup_torque = ctl->settings.max_torque + (ctl->settings.pid_kp * abs(state->rate_est) * PBIO_CONTROL_LOOP_TIME_MS * 2) / MS_PER_SECOND;

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
        pbio_count_integrator_stalled(&ctl->count_integrator, time_now, state->rate_est, ref->rate, ctl->settings.stall_time, ctl->settings.stall_rate_limit) :
        pbio_rate_integrator_stalled(&ctl->rate_integrator, time_now, state->rate_est, ref->rate, ctl->settings.stall_time, ctl->settings.stall_rate_limit);

    // Check if we are on target
    ctl->on_target = pbio_control_check_completion(ctl, ref->time, state, &ref_end);

    // Save (low-pass filtered) load for diagnostics
    ctl->load = (ctl->load * (100 - PBIO_CONTROL_LOOP_TIME_MS) + torque * PBIO_CONTROL_LOOP_TIME_MS) / 100;

    // Decide actuation based on whether control is on target.
    if (!ctl->on_target) {
        // If we're not on target yet, we keep actuating with
        // the PID torque value that has just been calculated.
        *actuation = PBIO_DCMOTOR_ACTUATION_TORQUE;
        *control = torque;
    } else {
        // If on target, decide what to do next using the on-completion type.
        switch (ctl->on_completion) {
            case PBIO_CONTROL_ON_COMPLETION_COAST:
                // Coast the motor and stop the control loop.
                *actuation = PBIO_DCMOTOR_ACTUATION_COAST;
                *control = 0;
                pbio_control_stop(ctl);
                break;
            case PBIO_CONTROL_ON_COMPLETION_BRAKE:
                // Passively brake and stop the control loop.
                *actuation = PBIO_DCMOTOR_ACTUATION_BRAKE;
                *control = 0;
                pbio_control_stop(ctl);
                break;
            case PBIO_CONTROL_ON_COMPLETION_COAST_SMART:
                // For smart coast, keep actuating (holding) briefly to enforce
                // standstill. It also gives some time for two subsequent
                // blocks to smoothly transition without going through coast.
                if (ref->time - ref_end.time < 100 * US_PER_MS) {
                    *actuation = PBIO_DCMOTOR_ACTUATION_TORQUE;
                    *control = torque;
                }
                // After that, coast the motor and stop the control loop.
                else {
                    *actuation = PBIO_DCMOTOR_ACTUATION_COAST;
                    *control = 0;
                    pbio_control_stop(ctl);
                }
                break;
            case PBIO_CONTROL_ON_COMPLETION_CONTINUE:
            // Fall through, same as hold.
            case PBIO_CONTROL_ON_COMPLETION_HOLD:
                // Holding position or continuing the trajectory just means we
                // have to keep actuating with the PID torque value that has
                // just been calculated.
                *actuation = PBIO_DCMOTOR_ACTUATION_TORQUE;
                *control = torque;

                // If we are getting here on completion of a timed command with
                // a stationary endpoint, convert it to a stationary angle
                // based command and hold it.
                if (pbio_control_type_is_time(ctl) && ref_end.rate == 0) {
                    pbio_control_start_hold_control(ctl, time_now, state->count);
                }
                break;
        }
    }

    // Log control data
    int32_t log_data[] = {
        ref->time - ctl->trajectory.t0,
        state->count,
        0,
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

// Stops the update loop from updating this controller.
void pbio_control_stop(pbio_control_t *ctl) {
    ctl->type = PBIO_CONTROL_NONE;
    ctl->on_target = true;
    ctl->stalled = false;
}

// Initializes the control state when creating the control object.
void pbio_control_reset(pbio_control_t *ctl) {

    // Stop the control loop update.
    pbio_control_stop(ctl);

    // Reset the previous on-completion state.
    ctl->on_completion = PBIO_CONTROL_ON_COMPLETION_COAST;

    // The on_completion state is the only persistent setting between
    // subsequent maneuvers, so nothing else needs to be reset explicitly.
}

pbio_error_t pbio_control_start_angle_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t target_count, int32_t target_rate, pbio_control_on_completion_t on_completion) {

    pbio_error_t err;

    // Set new maneuver action and stop type, and state
    ctl->on_completion = on_completion;
    ctl->on_target = false;

    // Common trajectory parameters for all cases covered here.
    pbio_trajectory_command_t command = {
        .angle_end = target_count,
        .speed_target = target_rate,
        .speed_max = ctl->settings.rate_max,
        .acceleration = ctl->settings.acceleration,
        .deceleration = ctl->settings.deceleration,
        .continue_running = on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    };

    // Given the control status, fill in remaining commands and get trajectory.
    if (!pbio_control_is_active(ctl)) {
        // If no control is ongoing, we just start from the measured state.
        command.time_start = time_now;
        command.angle_start = state->count;
        command.speed_start = state->rate_est;

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // Otherwise, If control is active, (re)start from the current
        // reference. This way the reference just branches off on a new
        // trajectory instead of falling back slightly, avoiding a speed drop.
        command.time_start = pbio_control_get_ref_time(ctl, time_now);
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&ctl->trajectory, command.time_start, &ref);
        command.angle_start = ref.count;
        command.speed_start = ref.rate;

        // Before we override the trajectory to renew it, get the starting
        // point of the current speed/angle segment of the reference. We may
        // need it below.
        pbio_trajectory_reference_t ref_vertex;
        pbio_trajectory_get_last_vertex(&ctl->trajectory, command.time_start, &ref_vertex);

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
            command.time_start = ref_vertex.time;
            command.angle_start = ref_vertex.count;
            command.speed_start = ref_vertex.rate;

            // Recalculate the trajectory from the shifted starting point.
            err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }

    // Reset PID control if needed
    if (!pbio_control_type_is_angle(ctl)) {
        // Get (again) the reference at current time, so we get the correct
        // value regardless of the command path followed above.
        pbio_trajectory_reference_t ref_new;
        pbio_trajectory_get_reference(&ctl->trajectory, time_now, &ref_new);

        // New angle maneuver, so reset the rate integrator
        int32_t integrator_max = pbio_control_settings_get_max_integrator(&ctl->settings);
        pbio_count_integrator_reset(&ctl->count_integrator, ref_new.time, ref_new.count, ref_new.count, integrator_max);

        // Reset load filter
        ctl->load = 0;
    }

    // Set the new control state
    ctl->type = PBIO_CONTROL_ANGLE;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_control_start_relative_angle_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t relative_target_count, int32_t target_rate, pbio_control_on_completion_t on_completion) {

    // First, we need to decide where the relative motion starts from.
    int32_t count_start;

    if (pbio_control_is_active(ctl)) {
        // If control is already active, restart from current reference.
        int32_t time_ref = pbio_control_get_ref_time(ctl, time_now);
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&ctl->trajectory, time_ref, &ref);
        count_start = ref.count;
    } else {
        // Control is inactive. We still have two options.
        // If the previous command used smart coast and we're still close to
        // its target, we want to start from there. This avoids accumulating
        // errors in programs that use mostly relative motions like run_angle.
        pbio_trajectory_reference_t prev_end;
        pbio_trajectory_get_endpoint(&ctl->trajectory, &prev_end);
        if (ctl->on_completion == PBIO_CONTROL_ON_COMPLETION_COAST_SMART &&
            abs(prev_end.count - state->count) < ctl->settings.count_tolerance * 2) {
            // We're close enough, so make the new target relative to the
            // endpoint of the last one.
            count_start = prev_end.count;
        } else {
            // No special cases apply, so the best we can do is just start from
            // the current state.
            count_start = state->count;
        }
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
        .time_start = pbio_control_get_ref_time(ctl, time_now),
        .angle_start = target_count,
        .angle_start_ext = 0,
        .speed_target = 0,
        .continue_running = false,
    };
    pbio_trajectory_make_constant(&ctl->trajectory, &command);
    // If called for the first time, set state and reset PID
    if (!pbio_control_type_is_angle(ctl)) {
        // Initialize or reset the PID control status for the given maneuver
        int32_t integrator_max = pbio_control_settings_get_max_integrator(&ctl->settings);
        pbio_count_integrator_reset(&ctl->count_integrator, time_now, command.angle_start, command.angle_start, integrator_max);

        // Reset load filter
        ctl->load = 0;
    }

    // This is an angular control maneuver
    ctl->type = PBIO_CONTROL_ANGLE;

    return PBIO_SUCCESS;
}


pbio_error_t pbio_control_start_timed_control(pbio_control_t *ctl, int32_t time_now, pbio_control_state_t *state, int32_t duration, int32_t target_rate, pbio_control_on_completion_t on_completion) {

    pbio_error_t err;

    if (on_completion == PBIO_CONTROL_ON_COMPLETION_COAST_SMART) {
        // For timed maneuvers, the end point has no meaning, so just coast.
        on_completion = PBIO_CONTROL_ON_COMPLETION_COAST;
    }

    // Set new maneuver action and stop type, and state
    ctl->on_completion = on_completion;
    ctl->on_target = false;

    // Common trajectory parameters for the cases covered here.
    pbio_trajectory_command_t command = {
        .time_start = time_now,
        .duration = duration,
        .speed_target = target_rate,
        .speed_max = ctl->settings.rate_max,
        .acceleration = ctl->settings.acceleration,
        .deceleration = ctl->settings.deceleration,
        .continue_running = on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    };

    // Given the control status, fill in remaining commands and get trajectory.
    if (!pbio_control_is_active(ctl)) {
        // If no control is ongoing, we just start from the measured state.
        command.time_start = time_now;
        command.angle_start = state->count;
        command.speed_start = state->rate_est;

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
        command.angle_start = ref.count;
        command.angle_start_ext = ref.count_ext;
        command.speed_start = ref.rate;

        // Before we override the trajectory to renew it, get the starting
        // point of the current speed/angle segment of the reference. We may
        // need it below.
        pbio_trajectory_reference_t ref_vertex;
        pbio_trajectory_get_last_vertex(&ctl->trajectory, command.time_start, &ref_vertex);

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
            command.time_start = ref_vertex.time;
            command.angle_start = ref_vertex.count;
            command.angle_start_ext = ref_vertex.count_ext;
            command.speed_start = ref_vertex.rate;

            // We shifted the start time into the past, so we must adjust
            // duration accordingly.
            command.duration += (time_now - ref_vertex.time);

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
