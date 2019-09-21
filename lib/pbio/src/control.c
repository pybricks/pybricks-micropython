// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <fixmath.h>

#include <pbio/servo.h>
#include <pbio/trajectory.h>

// If the controller reach the maximum duty cycle value, this shortcut sets the stalled flag when the speed is below the stall limit.
static void stall_set_flag_if_slow(pbio_control_stalled_t *stalled,
                                   rate_t rate_now,
                                   rate_t rate_limit,
                                   ustime_t stall_time,
                                   ustime_t stall_time_limit,
                                   pbio_control_stalled_t flag) {
    if (abs(rate_now) <= rate_limit && stall_time > stall_time_limit) {
        // If the speed is less than the specified limit, set stalled flag.
        *stalled |= flag;
    }
    else {
        // Otherwise we are not yet stalled, so clear this flag.
        *stalled &= ~flag;
    }
}

// Clear the specified stall flag
static void stall_clear_flag(pbio_control_stalled_t *stalled, pbio_control_stalled_t flag) {
    *stalled &= ~flag;
}


pbio_error_t control_update_angle_target(pbio_servo_t *mtr, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_control_after_stop_t *actuation_type, int32_t *control) {
    // Trajectory and setting shortcuts for this motor
    pbio_control_status_angular_t *status = &mtr->control.status_angular;
    duty_t max_duty = mtr->hbridge->max_duty_steps; // TODO: Make control property

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_ref, time_loop;
    count_t count_ref, count_err;
    rate_t rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Get the time at which we want to evaluate the reference position/velocities, for position based commands

    // In nominal operation, take the current time, minus the amount of time we have spent stalled
    if (status->ref_time_running) {
        time_ref = time_now - status->time_paused;
    }
    else {
    // When the motor stalls, we keep the time constant. This way, the position reference does
    // not continue to grow unboundedly, thus preventing a form of wind-up
        time_ref = status->time_stopped - status->time_paused;
    }

    // Get reference signals
    get_reference(time_ref, &mtr->control.trajectory, &count_ref, &rate_ref);

    // Position error. For position based commands, this is just the reference minus the current value
    count_err = count_ref - count_now;

    // For all commands, the speed error is simply the reference speed minus the current speed
    rate_err = rate_ref - rate_now;

    // Corresponding PD control signal
    duty_due_to_proportional = mtr->control.settings.pid_kp*count_err;
    duty_due_to_derivative = mtr->control.settings.pid_kd*rate_err;

    // Position anti-windup
    if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)) {
        // We are at the duty limit and we should prevent further position error "integration".
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(&mtr->control.stalled, rate_now, mtr->control.settings.stall_rate_limit, time_now - status->time_stopped, mtr->control.settings.stall_time, STALLED_PROPORTIONAL);
        // To prevent further integration, we should stop the timer if it is running
        if (status->ref_time_running) {
            // Then we must stop the time
            status->ref_time_running = false;
            // We save the time value reached now, to continue later
            status->time_stopped = time_now; // TODO: unset after clear stall?
        }
    }
    else{
        // Otherwise, the timer should be running, and we are not stalled
        stall_clear_flag(&mtr->control.stalled, STALLED_PROPORTIONAL);
        // So we should restart the time if it isn't already running
        if (!status->ref_time_running) {
            // Then we must restart the time
            status->ref_time_running = true;
            // We begin counting again from the stopped point
            status->time_paused += time_now - status->time_stopped;
        }
    }

    // Integrate position error
    if (status->ref_time_running) {
        time_loop = time_now - status->time_prev;
        status->err_integral += status->count_err_prev*time_loop;
    }
    status->count_err_prev = count_err;
    status->time_prev = time_now;

    // Duty cycle component due to integral position control
    duty_due_to_integral = (mtr->control.settings.pid_ki*(status->err_integral/US_PER_MS))/MS_PER_SECOND;

    // Integrator anti windup (stalled in the sense of integrators)
    // Limit the duty due to the integral, as well as the integral itself
    if (duty_due_to_integral > max_duty) {
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(&mtr->control.stalled, rate_now, mtr->control.settings.stall_rate_limit, time_now - status->time_stopped, mtr->control.settings.stall_time, STALLED_INTEGRAL);
        duty_due_to_integral = max_duty;
        status->err_integral = (US_PER_SECOND/mtr->control.settings.pid_ki)*max_duty;
    }
    else if (duty_due_to_integral < -max_duty) {
        stall_set_flag_if_slow(&mtr->control.stalled, rate_now, mtr->control.settings.stall_rate_limit, time_now - status->time_stopped, mtr->control.settings.stall_time, STALLED_INTEGRAL);
        duty_due_to_integral = -max_duty;
        status->err_integral = -(US_PER_SECOND/mtr->control.settings.pid_ki)*max_duty;
    }
    else {
        // Clear the integrator stall flag
        stall_clear_flag(&mtr->control.stalled, STALLED_INTEGRAL);
    }

    // Calculate duty signal
    duty = duty_due_to_proportional + duty_due_to_integral + duty_due_to_derivative;

    // Check if we are at the target and standing still, with slightly different end conditions for each mode
    if (mtr->control.action == RUN_TARGET &&
        // Maneuver is complete, time wise
        time_ref - mtr->control.trajectory.t3 >= 0 &&
        // Position is within the lower tolerated bound ...
        mtr->control.trajectory.th3 - mtr->control.settings.count_tolerance <= count_now &&
        // ... and the upper tolerated bound
        count_now <= mtr->control.trajectory.th3 + mtr->control.settings.count_tolerance &&
        // And the motor stands still.
        abs(rate_now) < mtr->control.settings.rate_tolerance)
    {
        // If so, we have reached our goal. Decide what to do next.
        if (mtr->control.after_stop == PBIO_MOTOR_STOP_HOLD) {
            // Holding just means that we continue the position control loop without changes
            *actuation_type = PBIO_ACTUATION_DUTY;
            *control = duty;

            // Altough we keep holding, the maneuver is completed
            mtr->state = PBIO_CONTROL_ANGLE_BACKGROUND;
        }
        else {
            // Otherwise, the next action is the specified after_stop, which is brake or coast
            *actuation_type = mtr->control.after_stop;
            *control = 0;
        }
    }
    else {
        // We are not stopping, so the actuation is to apply the calculated
        // control signal
        *actuation_type = PBIO_ACTUATION_DUTY;
        *control = duty;
    }
    return PBIO_SUCCESS;
}

pbio_error_t control_update_time_target(pbio_servo_t *mtr, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_control_after_stop_t *actuation_type, int32_t *control) {

    // Trajectory and setting shortcuts for this motor
    pbio_control_status_timed_t *status = &mtr->control.status_timed;
    duty_t max_duty = mtr->hbridge->max_duty_steps; // TODO: make control property

    // Declare time, positions, rates, and their reference value and error
    count_t count_ref, count_err;
    rate_t rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Get reference signals
    get_reference(time_now, &mtr->control.trajectory, &count_ref, &rate_ref);

    // For time based commands, we do not aim to drive to a specific position, but we use the
    // "proportional position control" as an exact way to implement "integral speed control".
    // The speed integral is simply the position, but the speed reference should stop integrating
    // while stalled, to prevent windup.
    if (status->speed_integrator_running) {
        // If integrator is active, it is the previously accumulated sum, plus the integral since its last restart
        count_err = status->speed_integrator + (count_ref - status->integrator_ref_start) - (count_now - status->integrator_start);
    }
    else {
        // Otherwise, it is just the previously accumulated sum and it doesn't integrate further
        count_err = status->speed_integrator;
    }

    // For all commands, the speed error is simply the reference speed minus the current speed
    rate_err = rate_ref - rate_now;

    // Corresponding PD control signal
    duty_due_to_proportional = mtr->control.settings.pid_kp*count_err;
    duty_due_to_derivative = mtr->control.settings.pid_kd*rate_err;

    // Position anti-windup
    // Check if proportional control exceeds the duty limit
    if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)) {
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(&mtr->control.stalled, rate_now, mtr->control.settings.stall_rate_limit, time_now - status->integrator_time_stopped, mtr->control.settings.stall_time, STALLED_PROPORTIONAL);
        // The integrator should NOT run.
        if (status->speed_integrator_running) {
            // If it is running, disable it
            status->speed_integrator_running = false;
            // Save the integrator state reached now, to continue when no longer stalled
            status->speed_integrator += count_ref - status->integrator_ref_start - count_now + status->integrator_start;
            // Store time at which speed integration is disabled
            status->integrator_time_stopped = time_now;
        }
    }
    else {
        stall_clear_flag(&mtr->control.stalled, STALLED_PROPORTIONAL);
        // The integrator SHOULD RUN.
        if (!status->speed_integrator_running) {
            // If it isn't running, enable it
            status->speed_integrator_running = true;
            // Begin integrating again from the current point
            status->integrator_ref_start = count_ref;
            status->integrator_start = count_now;
        }
    }

    // RUN || RUN_TIME || RUN_STALLED have no position integral control
    duty_due_to_integral = 0;
    stall_clear_flag(&mtr->control.stalled, STALLED_INTEGRAL);

    // Calculate duty signal
    duty = duty_due_to_proportional + duty_due_to_integral + duty_due_to_derivative;

    // Check if we are at the target and standing still, with slightly different end conditions for each mode
    if (
        // Conditions for RUN_TIME command: past the total duration of the timed command
        (
            mtr->control.action == RUN_TIME && time_now >= mtr->control.trajectory.t3
        )
        ||
        // Conditions for run_stalled commands: the motor is stalled in either proportional or integral sense
        (
            mtr->control.action == RUN_STALLED && mtr->control.stalled != STALLED_NONE
        )
    )
    {
        // Since we are stopping, the actuation is the after_stop action.
        *actuation_type = mtr->control.after_stop;

        // If that next action is holding, the corresponding signal is the
        // target position that must be held, which is the current position.
        if (*actuation_type == PBIO_MOTOR_STOP_HOLD) {
            *control = count_now;
        }
        // Otherwise, for coasting or braking, there is no further control signal.
        else {
            *control = 0;
        }
    }
    else {
        // We are not stopping, so the actuation is to apply the calculated
        // control signal
        *actuation_type = PBIO_ACTUATION_DUTY;
        *control = duty;
    }
    return PBIO_SUCCESS;
}

void control_init_angle_target(pbio_servo_t *mtr) {
    // TODO If already running, start from ref + set flag of original state

    // depending on wind up status, keep or finalize integrator state, plus maintain status

    pbio_control_status_angular_t *status = &mtr->control.status_angular;
    pbio_control_trajectory_t *trajectory = &mtr->control.trajectory;

    // For this new maneuver, we reset PID variables and related persistent control settings
    // If still running and restarting a new maneuver, however, we keep part of the PID status
    // in order to create a smooth transition from one maneuver to the next.
    // If no previous maneuver was active, just set these to zero.
    status->err_integral = 0;
    status->time_paused = 0;
    status->time_stopped = 0;
    status->time_prev = trajectory->t0;
    status->count_err_prev = 0;
    status->ref_time_running = true;
}

void control_init_time_target(pbio_servo_t *mtr) {
    pbio_control_status_timed_t *status = &mtr->control.status_timed;
    pbio_control_trajectory_t *trajectory = &mtr->control.trajectory;

    if (mtr->state == PBIO_CONTROL_TIME_BACKGROUND) {
        if (status->speed_integrator_running) {
            status->speed_integrator += trajectory->th0 - status->integrator_ref_start;
            status->integrator_ref_start = trajectory->th0;
        }
    }
    else {
        // old mode was passive, so start from zero,
        status->speed_integrator = 0;
        status->integrator_time_stopped = 0;
        status->speed_integrator_running = true;
        status->integrator_start = trajectory->th0;
        status->integrator_ref_start = trajectory->th0;
    }
}
