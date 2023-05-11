// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <stdlib.h>
#include <assert.h>

#include <pbdrv/clock.h>

#include <pbio/config.h>
#include <pbio/control.h>
#include <pbio/int_math.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>

/**
 * Gets the wall time in control unit time ticks (1e-4 seconds).
 *
 * @return                    Wall time in control ticks.
 */
uint32_t pbio_control_get_time_ticks(void) {
    _Static_assert(PBIO_TRAJECTORY_TICKS_PER_MS == 10, "time config must match clock resolution");
    return pbdrv_clock_get_100us();
}

/**
 * Checks if on completion type is active (control keeps going on completion).
 *
 * @param [in] on_completion  What to do on completion.
 * @return                    True if the completion type is active , else false.
 */
static bool pbio_control_on_completion_is_active(pbio_control_on_completion_t on_completion) {
    return on_completion == PBIO_CONTROL_ON_COMPLETION_HOLD ||
           on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE;
}

/**
 * Checks if on completion type is passive with smart mode (target position
 * will be preserved as starting point for next relative angle maneuver).
 *
 * @param [in] on_completion  What to do on completion.
 * @return                    True if the completion type is passive with smart mode.
 */
static bool pbio_control_on_completion_is_passive_smart(pbio_control_on_completion_t on_completion) {
    return on_completion == PBIO_CONTROL_ON_COMPLETION_COAST_SMART ||
           on_completion == PBIO_CONTROL_ON_COMPLETION_BRAKE_SMART;
}

/**
 * Converts passive on-completion type to passive actuation type.
 *
 * @param [in] on_completion  What to do on completion.
 * @return                    Matching passive actuation type.
 */
pbio_dcmotor_actuation_t pbio_control_passive_completion_to_actuation_type(pbio_control_on_completion_t on_completion) {

    assert(!pbio_control_on_completion_is_active(on_completion));

    if (on_completion == PBIO_CONTROL_ON_COMPLETION_COAST_SMART || on_completion == PBIO_CONTROL_ON_COMPLETION_COAST) {
        return PBIO_DCMOTOR_ACTUATION_COAST;
    }
    // Brake and smart brake are the only remaining allowed completion options,
    // so always return the matching actuation mode as brake.
    return PBIO_DCMOTOR_ACTUATION_BRAKE;
}

/**
 * Discards smart flag from on completion type.
 *
 * @param [in] on_completion  What to do on completion.
 * @return                    What to do on completion, discarding smart option.
 */
static pbio_control_on_completion_t pbio_control_on_completion_discard_smart(pbio_control_on_completion_t on_completion) {
    if (on_completion == PBIO_CONTROL_ON_COMPLETION_COAST_SMART) {
        return PBIO_CONTROL_ON_COMPLETION_COAST;
    }
    if (on_completion == PBIO_CONTROL_ON_COMPLETION_BRAKE_SMART) {
        return PBIO_CONTROL_ON_COMPLETION_BRAKE;
    }
    return on_completion;
}

static void pbio_control_status_set(pbio_control_t *ctl, pbio_control_status_flag_t flag, bool set) {
    ctl->status = set ? ctl->status | flag : ctl->status & ~flag;
}

static bool pbio_control_status_test(const pbio_control_t *ctl, pbio_control_status_flag_t flag) {
    return ctl->status & flag;
}

static bool pbio_control_check_completion(const pbio_control_t *ctl, uint32_t time, const pbio_control_state_t *state, const pbio_trajectory_reference_t *end) {

    // If no control is active, then all targets are complete.
    if (!pbio_control_is_active(ctl)) {
        return true;
    }

    // If stalling is the *objective*, stall state is completion state.
    if (ctl->type & PBIO_CONTROL_TYPE_FLAG_OBJECTIVE_IS_STALL) {
        return pbio_control_status_test(ctl, PBIO_CONTROL_STATUS_STALLED);
    }

    // If request to stop on stall, return if stalled but proceed with other checks.
    if (ctl->type & PBIO_CONTROL_TYPE_FLAG_STOP_ON_STALL && pbio_control_status_test(ctl, PBIO_CONTROL_STATUS_STALLED)) {
        return true;
    }

    // Check if we are passed the nominal maneuver time.
    bool time_completed = pbio_control_settings_time_is_later(time, end->time);

    if (pbio_control_type_is_time(ctl)) {
        // Infinite maneuvers are always done (should never block).
        if (pbio_trajectory_get_duration(&ctl->trajectory) >= pbio_control_time_ms_to_ticks(PBIO_TRAJECTORY_DURATION_FOREVER_MS)) {
            return true;
        }

        // Finite-time maneuvers are done when the full duration has passed.
        return time_completed;
    }

    // What remains now is to deal with angle-based maneuvers. As with time
    // based trajectories, we want at least the duration to pass, so return
    // false if time has not yet completed.
    if (!time_completed) {
        return false;
    }

    // For a nonzero final speed, we're done once we're at or past
    // the target, no matter the tolerances. Equivalently, we're done
    // once the sign of the angle error differs from the speed sign.
    int32_t position_remaining = pbio_angle_diff_mdeg(&end->position, &state->position);
    if (end->speed != 0) {
        return pbio_int_math_sign(position_remaining) != pbio_int_math_sign(end->speed);
    }

    // For zero final speed, we need to at least stand still, so return false
    // when we're still moving faster than the tolerance.
    if (pbio_int_math_abs(state->speed) > ctl->settings.speed_tolerance) {
        return false;
    }

    // Once we stand still, we're complete if the distance to the
    // target is equal to or less than the allowed tolerance.
    return pbio_int_math_abs(position_remaining) <= ctl->settings.position_tolerance;
}

/**
 * Gets the position, speed, and acceleration, on the reference trajectory.
 *
 * This expands the lower-level pbio_trajectory_get_reference(), by
 * additionally compensating for the stall state of the integrators.
 *
 * @param [in]  ctl              Control status structure.
 * @param [in]  time_now         Wall time (ticks).
 * @param [in]  state            Current control state.
 * @param [out] ref              Current reference, compensated for integrator state.
 *
 */
void pbio_control_get_reference(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, pbio_trajectory_reference_t *ref) {

    // Get reference state, compensating for any time shift in position control.
    pbio_trajectory_get_reference(&ctl->trajectory, pbio_control_get_ref_time(ctl, time_now), ref);

    // For timed control, further compensate reference position for the shift
    // in the speed integrator that may have been incurred due to load.
    if (pbio_control_type_is_time(ctl)) {
        int32_t position_error = pbio_angle_diff_mdeg(&ref->position, &state->position);
        int32_t position_error_used = pbio_speed_integrator_get_error(&ctl->speed_integrator, position_error);
        pbio_angle_add_mdeg(&ref->position, position_error_used - position_error);
    }
}

static int32_t pbio_control_get_pid_kp(const pbio_control_settings_t *settings, int32_t position_error, int32_t target_error, int32_t abs_command_speed) {

    // Reduced kp values are only needed for some motors under slow speed
    // conditions. For everything else, use the default kp value.
    if (abs_command_speed >= settings->pid_kp_low_speed_threshold || position_error == 0) {
        return settings->pid_kp;
    }

    // We only need a positive kp value, so work with absolute values
    // throughout this function to make things easy to follow.
    position_error = pbio_int_math_abs(position_error);
    target_error = pbio_int_math_abs(target_error);

    // Lowest kp value, used when steadily turning at slow speed.
    const int32_t kp_low = settings->pid_kp * settings->pid_kp_low_pct / 100;

    // Get equivalent kp value to produce a piece-wise affine (pwa) feedback in the
    // position error. It grows slower at first, and then at the configured rate.
    const int32_t kp_pwa = position_error <= settings->pid_kp_low_error_threshold ?
        // For small errors, feedback is linear in low kp value.
        kp_low :
        // Above the threshold, feedback grows with the default gain.
        settings->pid_kp - settings->pid_kp_low_error_threshold * (settings->pid_kp - kp_low) / position_error;

    // Proportional control saturates where the error leads to maximum actuation.
    // For errors any smaller than that,
    const int32_t saturation_lower = pbio_control_settings_div_by_gain(settings->actuation_max, settings->pid_kp);

    // Further away from the target, we can use the reduced value and still
    // guarantee maximum actuation, to avoid getting stuck.
    const int32_t saturation_upper = saturation_lower * 100 / settings->pid_kp_low_pct;

    // Get the kp value as constrained by the condition to use maxium actuation
    // close to the target to guarantee that we can always get there.
    int32_t kp_target;
    if (target_error < saturation_lower) {
        kp_target = settings->pid_kp;
    } else if (target_error > saturation_upper) {
        kp_target = kp_low;
    } else {
        // In between, we gradually shift towards the higher value as we get
        // closer to the final target to avoid a sudden transition.
        kp_target = kp_low + settings->pid_kp *
            (100 - settings->pid_kp_low_pct) * (saturation_upper - target_error) /
            (saturation_upper - saturation_lower) / 100;
    }

    // The most constrained objective is obtained by taking the highest value.
    return pbio_int_math_max(kp_pwa, kp_target);
}

/**
 * Updates the PID controller state to calculate the next actuation step.
 *
 * @param [in]   ctl              The control instance.
 * @param [in]   time_now         The wall time (ticks).
 * @param [in]   state            The current state of the system being controlled (control units).
 * @param [out]  ref              Computed reference point on the trajectory (control units).
 * @param [out]  actuation        Required actuation type.
 * @param [out]  control          The control output, which is the actuation payload (control units).
 * @param [inout] external_pause  Whether to force the controller to pause using external information (in), and
 *                                whether the controller still needs pausing according to its own state (out).
 */
void pbio_control_update(
    pbio_control_t *ctl,
    uint32_t time_now,
    const pbio_control_state_t *state,
    pbio_trajectory_reference_t *ref,
    pbio_dcmotor_actuation_t *actuation,
    int32_t *control,
    bool *external_pause) {

    // Get reference signals at the reference time point in the trajectory.
    // This compensates for any time we may have spent pausing when the motor was stalled.
    pbio_trajectory_get_reference(&ctl->trajectory, pbio_control_get_ref_time(ctl, time_now), ref);

    // Get reference point we want to be at in the end, to check for completion.
    pbio_trajectory_reference_t ref_end;
    pbio_trajectory_get_endpoint(&ctl->trajectory, &ref_end);

    // Get position and speed error
    int32_t position_error = pbio_angle_diff_mdeg(&ref->position, &state->position);
    int32_t speed_error = ref->speed - state->speed_estimate;

    // Calculate integral control errors, depending on control type.
    int32_t integral_error;
    int32_t position_error_used;
    int32_t target_error;
    if (pbio_control_type_is_position(ctl)) {
        // Get error to final position, used below to adjust controls near end.
        target_error = pbio_angle_diff_mdeg(&ref_end.position, &state->position);
        // Update count integral error and get current error state
        integral_error = pbio_position_integrator_update(&ctl->position_integrator, position_error, target_error);
        // For position control, the proportional term is the real position error.
        position_error_used = position_error;
    } else {
        // For time/speed based commands, the main error is speed. It integrates into a quantity with unit of position.
        // There is no count integral control, because we do not need a second order integrator for speed control.
        position_error_used = pbio_speed_integrator_get_error(&ctl->speed_integrator, position_error);
        integral_error = 0;
        // Speed maneuvers don't have a specific angle end target, so the error is infinite.
        target_error = INT32_MAX;
    }

    // Corresponding PID control signal
    int32_t pid_kp = pbio_control_get_pid_kp(&ctl->settings, position_error, target_error, pbio_trajectory_get_abs_command_speed(&ctl->trajectory));
    int32_t torque_proportional = pbio_control_settings_mul_by_gain(position_error_used, pid_kp);
    int32_t torque_derivative = pbio_control_settings_mul_by_gain(speed_error, ctl->settings.pid_kd);
    int32_t torque_integral = pbio_control_settings_mul_by_gain(integral_error, ctl->settings.pid_ki);

    // Total torque signal, capped by the actuation limit
    int32_t torque = pbio_int_math_clamp(torque_proportional + torque_integral + torque_derivative, ctl->settings.actuation_max_temporary);

    // This completes the computation of the control signal.
    // The next steps take care of handling windup, or triggering a stop if we are on target.

    // We want to stop building up further errors if we are at the proportional torque limit. So, we pause the trajectory
    // if we get at this limit. We wait a little longer though, to make sure it does not fall back to below the limit
    // within one sample, which we can predict using the current rate times the loop time, with a factor two tolerance.
    int32_t windup_margin = pbio_control_settings_mul_by_loop_time(pbio_int_math_abs(state->speed)) * 2;
    int32_t max_windup_torque = ctl->settings.actuation_max_temporary + pbio_control_settings_mul_by_gain(windup_margin, ctl->settings.pid_kp);

    // Speed value that is rounded to zero if small. This is used for a
    // direction error check below to avoid false reverses near zero.
    int32_t speed_for_direction_check = pbio_int_math_abs(state->speed) < ctl->settings.stall_speed_limit ? 0 : state->speed;

    // Position anti-windup: pause trajectory or integration if falling behind despite using maximum torque
    bool pause_integration =
        // Pause if proportional torque is beyond maximum windup torque:
        pbio_int_math_abs(torque_proportional) >= max_windup_torque &&
        // But not if we're trying to run in the other direction (else we can get unstuck by just reversing).
        pbio_int_math_sign(torque_proportional) != -pbio_int_math_sign(ref->speed - speed_for_direction_check) &&
        // But not if we should be accelerating in the other direction (else we can get unstuck by just reversing).
        pbio_int_math_sign(torque_proportional) != -pbio_int_math_sign(ref->acceleration);

    // Position anti-windup in case of angle control (accumulated position error may not get too high)
    if (pbio_control_type_is_position(ctl)) {
        if (pause_integration || *external_pause) {
            // We are at the torque limit and we should prevent further position error integration.
            pbio_position_integrator_pause(&ctl->position_integrator, time_now);
        } else {
            // Not at the limit so continue integrating errors
            pbio_position_integrator_resume(&ctl->position_integrator, time_now);
        }
    }
    // Position anti-windup in case of timed speed control (speed integral may not get too high)
    else {
        if (pause_integration || *external_pause) {
            // We are at the torque limit and we should prevent further speed error integration.
            pbio_speed_integrator_pause(&ctl->speed_integrator, time_now, position_error);
        } else {
            // Not at the limit so continue integrating errors
            pbio_speed_integrator_resume(&ctl->speed_integrator, position_error);
        }
    }

    // Informs caller if pausing is still needed according to this controller's state.
    *external_pause = pause_integration;

    // Check if controller is stalled, and set the status.
    pbio_control_status_set(ctl, PBIO_CONTROL_STATUS_STALLED,
        pbio_control_type_is_position(ctl) ?
        pbio_position_integrator_stalled(&ctl->position_integrator, time_now, state->speed, ref->speed) :
        pbio_speed_integrator_stalled(&ctl->speed_integrator, time_now, state->speed, ref->speed));

    // Check if we are on target, and set the status.
    pbio_control_status_set(ctl, PBIO_CONTROL_STATUS_COMPLETE,
        pbio_control_check_completion(ctl, ref->time, state, &ref_end));

    // Save (low-pass filtered) load for diagnostics
    ctl->pid_average = (ctl->pid_average * (100 - PBIO_CONFIG_CONTROL_LOOP_TIME_MS) + torque * PBIO_CONFIG_CONTROL_LOOP_TIME_MS) / 100;

    // Decide actuation based on control status.
    if (// Not on target yet, so keep actuating.
        !pbio_control_status_test(ctl, PBIO_CONTROL_STATUS_COMPLETE) ||
        // Active completion type, so keep actuating.
        pbio_control_on_completion_is_active(ctl->on_completion) ||
        // Smart passive mode, and we're only just complete, so keep actuating.
        // This ensures that any subsequent user command can pick up from here
        // without resetting any controllers. This avoids accumulating errors
        // in sequential relative maneuvers.
        (pbio_control_on_completion_is_passive_smart(ctl->on_completion) &&
         !pbio_control_settings_time_is_later(ref->time, ref_end.time + ctl->settings.smart_passive_hold_time))) {
        // Keep actuating, so apply calculated PID torque value.
        *actuation = PBIO_DCMOTOR_ACTUATION_TORQUE;
        *control = torque;
    } else {
        // No more control is needed, so switch to passive mode.
        *actuation = pbio_control_passive_completion_to_actuation_type(ctl->on_completion);
        *control = 0;
        pbio_control_stop(ctl);
    }

    // Handling hold after running for time requires an extra step because
    // it can only be done by starting a new position based command.
    if (pbio_control_status_test(ctl, PBIO_CONTROL_STATUS_COMPLETE) &&
        pbio_control_type_is_time(ctl) &&
        ctl->on_completion == PBIO_CONTROL_ON_COMPLETION_HOLD) {
        // Use current state as target for holding.
        int32_t target = pbio_control_settings_ctl_to_app_long(&ctl->settings, &state->position);
        pbio_control_start_position_control_hold(ctl, time_now, target);
    }

    // Optionally log control data.
    if (pbio_logger_is_active(&ctl->log)) {

        // For speed control, we use a reference adjusted for pausing. This is
        // accounted for above, but here we need it in angle units for logging.
        pbio_angle_t ref_position_log = ref->position;
        pbio_angle_add_mdeg(&ref_position_log, position_error_used - position_error);

        int32_t log_data[] = {
            // Column 0: Log time (added by logger).
            // Column 1: Time since start of trajectory.
            ref->time - ctl->trajectory.start.time,
            // Column 2: Position in application units.
            pbio_control_settings_ctl_to_app_long(&ctl->settings, &state->position),
            // Column 3: Speed in application units.
            pbio_control_settings_ctl_to_app(&ctl->settings, state->speed),
            // Column 4: Actuation type (LSB 0--1), stall state (LSB 2), on target (LSB 3), pause integration (LSB 4).
            *actuation | (ctl->status << 2) | ((int32_t)pause_integration << 4),
            // Column 5: Actuation payload, e.g. torque.
            *control,
            // Column 6: Reference position in application units.
            pbio_control_settings_ctl_to_app_long(&ctl->settings, &ref_position_log),
            // Column 7: Reference speed in application units.
            pbio_control_settings_ctl_to_app(&ctl->settings, ref->speed),
            // Column 8: Estimated position in application units.
            pbio_control_settings_ctl_to_app_long(&ctl->settings, &state->position_estimate),
            // Column 9: Estimated speed in application units.
            pbio_control_settings_ctl_to_app(&ctl->settings, state->speed_estimate),
            // Column 10: P term of PID control in (uNm).
            torque_proportional,
            // Column 12: I term of PID control in (uNm).
            torque_integral,
            // Column 13: D term of PID control in (uNm).
            torque_derivative,
        };
        pbio_logger_add_row(&ctl->log, log_data);
    }
}

/**
 * Stops (but not resets) the update loop from updating this controller. This
 * is normally called when a motor coasts or brakes.
 *
 * @param [in]  ctl         Control status structure.
 */
void pbio_control_stop(pbio_control_t *ctl) {
    ctl->type = PBIO_CONTROL_TYPE_NONE;
    pbio_control_status_set(ctl, PBIO_CONTROL_STATUS_COMPLETE, true);
    pbio_control_status_set(ctl, PBIO_CONTROL_STATUS_STALLED, false);
    ctl->pid_average = 0;
}

/**
 * Sets the control type for the new maneuver and initializes the corresponding
 * control status.
 *
 * @param [in]  ctl            The control instance.
 * @param [in]  time_now       The wall time (ticks).
 * @param [in]  type           Control type for the next maneuver.
 * @param [in]  on_completion  What to do when reaching the target position.
 */
static void pbio_control_set_control_type(pbio_control_t *ctl, uint32_t time_now, pbio_control_type_t type, pbio_control_on_completion_t on_completion) {

    // Setting none control type is the same as stopping.
    if ((type & PBIO_CONTROL_TYPE_MASK) == PBIO_CONTROL_TYPE_NONE) {
        pbio_control_stop(ctl);
        return;
    }

    // Set on completion action for this maneuver.
    ctl->on_completion = on_completion;

    // Reset maximum actuation value used for this run.
    ctl->settings.actuation_max_temporary = ctl->settings.actuation_max;

    // Reset done state. It will get the correct value during the next control
    // update. REVISIT: Evaluate it here.
    pbio_control_status_set(ctl, PBIO_CONTROL_STATUS_COMPLETE, false);

    // Exit if control type already set.
    if (ctl->type == type) {
        return;
    }

    // Reset stall state. It will get the correct value during the next control
    // update. REVISIT: Evaluate it here.
    pbio_control_status_set(ctl, PBIO_CONTROL_STATUS_STALLED, false);

    // Reset integrator for new control type.
    if ((type & PBIO_CONTROL_TYPE_MASK) == PBIO_CONTROL_TYPE_POSITION) {
        // If the new type is position, reset position integrator.
        pbio_position_integrator_reset(&ctl->position_integrator, &ctl->settings, time_now);
    } else {
        // If the new type is timed, reset speed integrator.
        pbio_speed_integrator_reset(&ctl->speed_integrator, &ctl->settings);
    }

    // Set the given type.
    ctl->type = type;
}

/**
 * Resets and initializes the control state. This is called when a device that
 * uses this controller is first initialized or when it is disconnected.
 *
 * @param [in]  ctl         Control status structure.
 */
void pbio_control_reset(pbio_control_t *ctl) {

    // Stop the control loop update.
    pbio_control_stop(ctl);

    // Reset the previous on-completion state.
    ctl->on_completion = PBIO_CONTROL_ON_COMPLETION_COAST;

    // The on_completion state is the only persistent setting between
    // subsequent maneuvers, so nothing else needs to be reset explicitly.
}

static pbio_error_t _pbio_control_start_position_control(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, const pbio_angle_t *target, int32_t speed, pbio_control_on_completion_t on_completion, bool allow_trajectory_shift) {

    pbio_error_t err;

    // Common trajectory parameters for all cases covered here.
    pbio_trajectory_command_t command = {
        .position_end = *target,
        .speed_target = speed == 0 ? ctl->settings.speed_default : speed,
        .speed_max = ctl->settings.speed_max,
        .acceleration = ctl->settings.acceleration,
        .deceleration = ctl->settings.deceleration,
        .continue_running = on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    };


    // Given the control status, fill in remaining commands and get trajectory.
    if (!pbio_control_is_active(ctl)) {
        // If no control is ongoing, we just start from the measured state.
        command.time_start = time_now;
        command.position_start = state->position;
        command.speed_start = state->speed;

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else if (pbio_control_type_is_time(ctl)) {
        // If timed control is ongoing, start from its current reference,
        // accounting for its integrator windup.
        pbio_trajectory_reference_t ref;
        pbio_control_get_reference(ctl, time_now, state, &ref);
        command.time_start = ref.time;
        command.position_start = ref.position;
        command.speed_start = ref.speed;

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // Otherwise, if position control is active, (re)start from the current
        // reference. This way the reference just branches off on a new
        // trajectory instead of falling back slightly, avoiding a speed drop.
        pbio_trajectory_reference_t ref;
        pbio_control_get_reference(ctl, time_now, state, &ref);
        command.time_start = ref.time;
        command.position_start = ref.position;
        command.speed_start = ref.speed;

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
        if (ctl->trajectory.a0 == ref.acceleration && allow_trajectory_shift) {

            // Update command with shifted starting point, equal to ongoing
            // maneuver.
            command.time_start = ref_vertex.time;
            command.position_start = ref_vertex.position;
            command.speed_start = ref_vertex.speed;

            // Recalculate the trajectory from the shifted starting point.
            err = pbio_trajectory_new_angle_command(&ctl->trajectory, &command);
            if (err != PBIO_SUCCESS) {
                return err;
            }
        }
    }

    // Activate control type and reset integrators if needed.
    pbio_control_set_control_type(ctl, time_now, PBIO_CONTROL_TYPE_POSITION, on_completion);

    return PBIO_SUCCESS;
}

/**
 * Starts the controller to run to a given target position.
 *
 * In a servo application, this means running to a target angle.
 *
 * @param [in]  ctl            The control instance.
 * @param [in]  time_now       The wall time (ticks).
 * @param [in]  state          The current state of the system being controlled (control units).
 * @param [in]  position       The target position to run to (application units).
 * @param [in]  speed          The top speed on the way to the target (application units). The sign is ignored. If zero, default speed is used.
 * @param [in]  on_completion  What to do when reaching the target position.
 * @return                     Error code.
 */
pbio_error_t pbio_control_start_position_control(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, int32_t position, int32_t speed, pbio_control_on_completion_t on_completion) {

    // Convert target position to control units.
    pbio_angle_t target;
    pbio_control_settings_app_to_ctl_long(&ctl->settings, position, &target);

    // Start position control in control units.
    return _pbio_control_start_position_control(ctl, time_now, state, &target, pbio_control_settings_app_to_ctl(&ctl->settings, speed), on_completion, true);
}

/**
 * Starts the controller to run by a given distance.
 *
 * In a servo application, this means running by the given angle.
 *
 * This function computes what the new target position will be, and then
 * calls pbio_control_start_position_control to get there.
 *
 * @param [in]  ctl                    The control instance.
 * @param [in]  time_now               The wall time (ticks).
 * @param [in]  state                  The current state of the system being controlled (control units).
 * @param [in]  distance               The distance to run by (application units).
 * @param [in]  speed                  The top speed on the way to the target (application units). Negative speed flips the distance sign.
 * @param [in]  on_completion          What to do when reaching the target position.
 * @param [in]  allow_trajectory_shift Whether trajectory may be time-shifted for better performance in tight loops (true) or not (false).
 * @return                             Error code.
 */
pbio_error_t pbio_control_start_position_control_relative(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, int32_t distance, int32_t speed, pbio_control_on_completion_t on_completion, bool allow_trajectory_shift) {

    // Convert distance to control units.
    pbio_angle_t increment;
    pbio_control_settings_app_to_ctl_long(&ctl->settings, (speed < 0 ? -distance : distance), &increment);

    // We need to decide where the relative motion starts from, and use that
    // to compute the position target by adding the increment.
    pbio_angle_t target;

    if (pbio_control_is_active(ctl)) {
        // If control is already active, restart from current reference.
        pbio_trajectory_reference_t ref;
        pbio_control_get_reference(ctl, time_now, state, &ref);
        pbio_angle_sum(&ref.position, &increment, &target);
    } else {
        // Control is inactive. We still have two options.
        // If the previous command used smart coast and we're still close to
        // its target, we want to start from there. This avoids accumulating
        // errors in programs that use mostly relative motions like run_angle.
        pbio_trajectory_reference_t prev_end;
        pbio_trajectory_get_endpoint(&ctl->trajectory, &prev_end);
        if (pbio_control_on_completion_is_passive_smart(ctl->on_completion) &&
            pbio_angle_diff_is_small(&prev_end.position, &state->position) &&
            pbio_int_math_abs(pbio_angle_diff_mdeg(&prev_end.position, &state->position)) < ctl->settings.position_tolerance * 2) {
            // We're close enough, so make the new target relative to the
            // endpoint of the last one.
            pbio_angle_sum(&prev_end.position, &increment, &target);
        } else {
            // No special cases apply, so the best we can do is just start from
            // the current state.
            pbio_angle_sum(&state->position, &increment, &target);
        }
    }

    return _pbio_control_start_position_control(ctl, time_now, state, &target, pbio_control_settings_app_to_ctl(&ctl->settings, speed), on_completion, allow_trajectory_shift);
}

/**
 * Starts the controller and holds at the given position.
 *
 * This is similar to starting position control, but it skips the trajectory
 * computation and just sets the reference to the target position right away.
 *
 * @param [in]  ctl             The control instance.
 * @param [in]  time_now        The wall time (ticks).
 * @param [in]  position        The target position to hold (application units).
 * @return                      Error code.
 */
pbio_error_t pbio_control_start_position_control_hold(pbio_control_t *ctl, uint32_t time_now, int32_t position) {

    // Compute new maneuver based on user argument, starting from the initial state
    pbio_trajectory_command_t command = {
        .time_start = pbio_control_get_ref_time(ctl, time_now),
        .speed_target = 0,
        .continue_running = false,
    };
    pbio_control_settings_app_to_ctl_long(&ctl->settings, position, &command.position_start);
    pbio_control_settings_app_to_ctl_long(&ctl->settings, position, &command.position_end);

    // Holding means staying at a constant trajectory.
    pbio_trajectory_make_constant(&ctl->trajectory, &command);

    // Activate control type and reset integrators if needed.
    pbio_control_set_control_type(ctl, time_now, PBIO_CONTROL_TYPE_POSITION, PBIO_CONTROL_ON_COMPLETION_HOLD);

    return PBIO_SUCCESS;
}

/**
 * Starts the controller to run for a given amount of time.
 *
 * @param [in]  ctl             The control instance.
 * @param [in]  time_now        The wall time (ticks).
 * @param [in]  state           The current state of the system being controlled (control units).
 * @param [in]  duration        For how long to run (ms).
 * @param [in]  speed           The top speed (application units). Negative speed means reverse.
 * @param [in]  on_completion   What to do when duration is over.
 * @return                      Error code.
 */
pbio_error_t pbio_control_start_timed_control(pbio_control_t *ctl, uint32_t time_now, const pbio_control_state_t *state, uint32_t duration, int32_t speed, pbio_control_on_completion_t on_completion) {

    pbio_error_t err;

    // For timed maneuvers, being "smart" by remembering the position endpoint
    // does nothing useful, so discard it to keep only the passive actuation type.
    on_completion = pbio_control_on_completion_discard_smart(on_completion);

    // Common trajectory parameters for the cases covered here.
    pbio_trajectory_command_t command = {
        .time_start = time_now,
        .duration = pbio_control_time_ms_to_ticks(duration),
        .speed_target = pbio_control_settings_app_to_ctl(&ctl->settings, speed),
        .speed_max = ctl->settings.speed_max,
        .acceleration = ctl->settings.acceleration,
        .deceleration = ctl->settings.deceleration,
        .continue_running = on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE,
    };

    // Given the control status, fill in remaining commands and get trajectory.
    if (!pbio_control_is_active(ctl)) {
        // If no control is ongoing, we just start from the measured state.
        command.time_start = time_now;
        command.position_start = state->position;
        command.speed_start = state->speed;

        // With the command fully populated, we can calculate the trajectory.
        err = pbio_trajectory_new_time_command(&ctl->trajectory, &command);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // Otherwise, If control is active, (re)start from the current
        // reference. This way the reference just branches off on a new
        // trajectory instead of falling back slightly, avoiding a speed drop.
        // NB: We do not compensate for the speed integrator offset here using
        // pbio_control_get_reference() since the new time based maneuver
        // proceeds to use the same one.
        uint32_t time_ref = pbio_control_get_ref_time(ctl, time_now);
        pbio_trajectory_reference_t ref;
        pbio_trajectory_get_reference(&ctl->trajectory, time_ref, &ref);
        command.position_start = ref.position;
        command.speed_start = ref.speed;

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
            command.position_start = ref_vertex.position;
            command.speed_start = ref_vertex.speed;

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

    // Activate control type and reset integrators if needed.
    pbio_control_set_control_type(ctl, time_now, PBIO_CONTROL_TYPE_TIMED, on_completion);

    return PBIO_SUCCESS;
}



/**
 * Gets the time at which to evaluate the reference trajectory by compensating
 * the wall time by the amount of time spent stalling during which a position
 * based trajectory does not progress.
 *
 * @param [in]  ctl         Control status structure.
 * @param [in]  time_now    Wall time (ticks).
 * @return int32_t          Time (ticks) on the trajectory curve.
 */
uint32_t pbio_control_get_ref_time(const pbio_control_t *ctl, uint32_t time_now) {
    // Angle controllers may pause the time so the reference position does not
    // keep accumulating while the controller is stuck.
    if (pbio_control_type_is_position(ctl)) {
        return pbio_position_integrator_get_ref_time(&ctl->position_integrator, time_now);
    }
    // In all other cases, it is just the current time.
    return time_now;
}

/**
 * Checks if the controller is currently active.
 *
 * @param [in]  ctl             The control instance.
 * @return                      True if active (position or time), false if not.
 */
bool pbio_control_is_active(const pbio_control_t *ctl) {
    return (ctl->type & PBIO_CONTROL_TYPE_MASK) != PBIO_CONTROL_TYPE_NONE;
}

/**
 * Checks if the controller is currently doing position control.
 *
 * @param [in]  ctl             The control instance.
 * @return                      True if position control is active, false if not.
 */
bool pbio_control_type_is_position(const pbio_control_t *ctl) {
    return (ctl->type & PBIO_CONTROL_TYPE_MASK) == PBIO_CONTROL_TYPE_POSITION;
}

/**
 * Checks if the controller is currently doing timed control.
 *
 * @param [in]  ctl             The control instance.
 * @return                      True if timed control is active, false if not.
 */
bool pbio_control_type_is_time(const pbio_control_t *ctl) {
    return (ctl->type & PBIO_CONTROL_TYPE_MASK) == PBIO_CONTROL_TYPE_TIMED;
}

/**
 * Checks if the controller is stalled and for how long.
 *
 * @param [in]  ctl             The control instance.
 * @param [out] stall_duration  For how long the controller has stalled (ticks).
 * @return                      True if controller is stalled, false if not.
 */
bool pbio_control_is_stalled(const pbio_control_t *ctl, uint32_t *stall_duration) {

    // Return false if no control is active or if we're not stalled.
    if (!pbio_control_is_active(ctl) || !pbio_control_status_test(ctl, PBIO_CONTROL_STATUS_STALLED)) {
        *stall_duration = 0;
        return false;
    }

    // Get time since stalling began.
    uint32_t time_pause_begin = pbio_control_type_is_position(ctl) ? ctl->position_integrator.time_pause_begin : ctl->speed_integrator.time_pause_begin;
    *stall_duration = pbio_control_get_time_ticks() - time_pause_begin;

    return true;
}

/**
 * Checks if the controller is done.
 *
 * For trajectories with a stationary endpoint, done means on target.
 *
 * @param [in]  ctl             The control instance.
 * @return                      True if the controller is done, false if not.
 */
bool pbio_control_is_done(const pbio_control_t *ctl) {
    return !pbio_control_is_active(ctl) || pbio_control_status_test(ctl, PBIO_CONTROL_STATUS_COMPLETE);
}
