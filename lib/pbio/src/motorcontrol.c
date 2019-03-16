// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbio/motor.h>
#include <pbio/motorref.h>
#include <stdlib.h>
#include <math.h>

#include "sys/clock.h"

// If the controller reach the maximum duty cycle value, this shortcut sets the stalled flag when the speed is below the stall limit.
void stall_set_flag_if_slow(stalled_status_t *stalled,
                            rate_t rate_now,
                            rate_t rate_limit,
                            ustime_t stall_time,
                            ustime_t stall_time_limit,
                            stalled_status_t flag) {
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
void stall_clear_flag(stalled_status_t *stalled, stalled_status_t flag) {
    *stalled &= ~flag;
}

pbio_error_t control_update_angle_target(pbio_port_t port) {

    // Trajectory and setting shortcuts for this motor
    pbio_motor_maneuver_t *maneuver = &motor[PORT_TO_IDX(port)].maneuver;
    pbio_motor_trajectory_t *traject = &maneuver->trajectory;
    pbio_motor_settings_t *settings = &motor[PORT_TO_IDX(port)].settings;
    pbio_motor_angular_control_status_t *status = &motor[PORT_TO_IDX(port)].angular_control_status;
    stalled_status_t *stalled = &motor[PORT_TO_IDX(port)].stalled;
    duty_t max_duty = settings->max_duty_steps;

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_now, time_ref, time_loop;
    count_t count_now, count_ref, count_err;
    rate_t rate_now, rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Read current state of this motor: current time, speed, and position
    time_now = clock_usecs();
    pbio_motor_get_encoder_count(port, &count_now);
    pbio_motor_get_encoder_rate(port, &rate_now);

    // Get the time at which we want to evaluate the reference position/velocities, for position based commands

    // In nominal operation, take the current time, minus the amount of time we have spent stalled
    if (status->windup_status == TIME_RUNNING) {
        time_ref = time_now - status->time_paused;
    }
    else {
    // When the motor stalls, we keep the time constant. This way, the position reference does
    // not continue to grow unboundedly, thus preventing a form of wind-up
        time_ref = status->time_stopped - status->time_paused;
    }

    // Get reference signals
    get_reference(time_ref, traject, &count_ref, &rate_ref);

    // Position error. For position based commands, this is just the reference minus the current value
    count_err = count_ref - count_now;

    // For all commands, the speed error is simply the reference speed minus the current speed
    rate_err = rate_ref - rate_now;

    // Corresponding PD control signal
    duty_due_to_proportional = settings->pid_kp*count_err;
    duty_due_to_derivative = settings->pid_kd*rate_err;

    // Position anti-windup
    if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)) {
        // We are at the duty limit and we should prevent further position error "integration".
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(stalled, rate_now, settings->stall_rate_limit, time_now - status->time_stopped, settings->stall_time, STALLED_PROPORTIONAL);
        // To prevent further integration, we should stop the timer if it is running
        if (status->windup_status == TIME_RUNNING) {
            // Then we must stop the time
            status->windup_status = TIME_PAUSED;
            // We save the time value reached now, to continue later
            status->time_stopped = time_now;
        }
    }
    else{
        // Otherwise, the timer should be running, and we are not stalled
        stall_clear_flag(stalled, STALLED_PROPORTIONAL);
        // So we should restart the time if it isn't already running
        if (status->windup_status == TIME_PAUSED) {
            // Then we must restart the time
            status->windup_status = TIME_RUNNING;
            // We begin counting again from the stopped point
            status->time_paused += time_now - status->time_stopped;
        }
    }

    // Integrate position error
    if (status->windup_status == TIME_RUNNING) {
        time_loop = time_now - status->time_prev;
        status->err_integral += status->count_err_prev*time_loop;
    }
    status->count_err_prev = count_err;
    status->time_prev = time_now;

    // Duty cycle component due to integral position control
    duty_due_to_integral = (settings->pid_ki*(status->err_integral/US_PER_MS))/MS_PER_SECOND;

    // Integrator anti windup (stalled in the sense of integrators)
    // Limit the duty due to the integral, as well as the integral itself
    if (duty_due_to_integral > max_duty) {
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(stalled, rate_now, settings->stall_rate_limit, time_now - status->time_stopped, settings->stall_time, STALLED_INTEGRAL);
        duty_due_to_integral = max_duty;
        status->err_integral = (US_PER_SECOND/settings->pid_ki)*max_duty;
    }
    else if (duty_due_to_integral < -max_duty) {
        stall_set_flag_if_slow(stalled, rate_now, settings->stall_rate_limit, time_now - status->time_stopped, settings->stall_time, STALLED_INTEGRAL);
        duty_due_to_integral = -max_duty;
        status->err_integral = -(US_PER_SECOND/settings->pid_ki)*max_duty;
    }
    else {
        // Clear the integrator stall flag
        stall_clear_flag(stalled, STALLED_INTEGRAL);
    }
    // Load duty
    status->load_duty = duty_due_to_integral;

    // Calculate duty signal
    duty = duty_due_to_proportional + duty_due_to_integral + duty_due_to_derivative;

    // Check if we are at the target and standing still, with slightly different end conditions for each mode
    if (
        (maneuver->action == RUN_TARGET) &&
            (
                // Maneuver is complete, time wise
                time_ref >= traject->t3 &&
                // Position is within the lower tolerated bound ...
                traject->th3 - settings->count_tolerance <= count_now &&
                // ... and the upper tolerated bound
                count_now <= traject->th3 + settings->count_tolerance &&
                // And the motor stands still.
                abs(rate_now) < settings->rate_tolerance
            )
        )
    {
    // If so, we have reached our goal. We can keep running this loop in order to hold this position.
    // But if brake or coast was specified as the afer_stop, we trigger that. Also clear the running flag to stop waiting for completion.
        if (maneuver->after_stop == PBIO_MOTOR_STOP_COAST) {
            // Coast the motor
            pbio_motor_coast(port);
        }
        else if (maneuver->after_stop == PBIO_MOTOR_STOP_BRAKE) {
            // Brake the motor
            pbio_motor_brake(port);
        }
        else if (maneuver->after_stop == PBIO_MOTOR_STOP_HOLD) {
            // Hold the motor. In position based control, holding just means that we continue the position control loop without changes
            pbio_motor_set_duty_cycle_sys(port, duty);

            // Altough we keep holding, the maneuver is completed
            motor[PORT_TO_IDX(port)].state = PBIO_MOTOR_CONTROL_TRACKING;

        }
    }
    // If we are not standing still at a target yet, actuate with the calculated signal
    else {
        pbio_motor_set_duty_cycle_sys(port, duty);
    }
    return PBIO_SUCCESS; // TODO catch and return errors for all above IO tasks
}

pbio_error_t control_update_time_target(pbio_port_t port) {

    // Trajectory and setting shortcuts for this motor
    pbio_motor_maneuver_t *maneuver = &motor[PORT_TO_IDX(port)].maneuver;
    pbio_motor_trajectory_t *traject = &maneuver->trajectory;
    pbio_motor_settings_t *settings = &motor[PORT_TO_IDX(port)].settings;
    pbio_motor_timed_control_status_t *status = &motor[PORT_TO_IDX(port)].timed_control_status;
    stalled_status_t *stalled = &motor[PORT_TO_IDX(port)].stalled;
    duty_t max_duty = settings->max_duty_steps;

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_now, time_ref;
    count_t count_now, count_ref, count_err;
    rate_t rate_now, rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Read current state of this motor: current time, speed, and position
    time_now = clock_usecs();
    pbio_motor_get_encoder_count(port, &count_now);
    pbio_motor_get_encoder_rate(port, &rate_now);

    // Get the time at which we want to evaluate the reference position/velocities.
    // For time based commands, we never pause the time; it is just the current time
    time_ref = time_now;

    // Get reference signals
    get_reference(time_ref, traject, &count_ref, &rate_ref);

    // else: RUN || RUN_TIME || RUN_STALLED
    // For time based commands, we do not aim to drive to a specific position, but we use the
    // "proportional position control" as an exact way to implement "integral speed control".
    // The speed integral is simply the position, but the speed reference should stop integrating
    // while stalled, to prevent windup.
    if (status->windup_status == SPEED_INTEGRATOR_RUNNING) {
        // If integrator is active, it is the previously accumulated sum, plus the integral since its last restart
        count_err = status->speed_integrator + count_ref - status->integrator_ref_start - count_now + status->integrator_start;
    }
    else {
        // Otherwise, it is just the previously accumulated sum and it doesn't integrate further
        count_err = status->speed_integrator;
    }

    // For all commands, the speed error is simply the reference speed minus the current speed
    rate_err = rate_ref - rate_now;

    // Corresponding PD control signal
    duty_due_to_proportional = settings->pid_kp*count_err;
    duty_due_to_derivative = settings->pid_kd*rate_err;

    // Position anti-windup (RUN || RUN_TIME || RUN_STALLED)
    // Check if proportional control exceeds the duty limit
    if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)) {
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(stalled, rate_now, settings->stall_rate_limit, time_now - status->integrator_time_stopped, settings->stall_time, STALLED_PROPORTIONAL);
        // The integrator should NOT run.
        if (status->windup_status == SPEED_INTEGRATOR_RUNNING) {
            // If it is running, disable it
            status->windup_status = SPEED_INTEGRATOR_PAUSED;
            // Save the integrator state reached now, to continue when no longer stalled
            status->speed_integrator += count_ref - status->integrator_ref_start - count_now + status->integrator_start;
            // Store time at which speed integration is disabled
            status->integrator_time_stopped = time_now;
        }
    }
    else {
        stall_clear_flag(stalled, STALLED_PROPORTIONAL);
        // The integrator SHOULD RUN.
        if (status->windup_status == SPEED_INTEGRATOR_PAUSED) {
            // If it isn't running, enable it
            status->windup_status = SPEED_INTEGRATOR_RUNNING;
            // Begin integrating again from the current point
            status->integrator_ref_start = count_ref;
            status->integrator_start = count_now;
        }
    }

    // RUN || RUN_TIME || RUN_STALLED have no position integral control
    duty_due_to_integral = 0;
    stall_clear_flag(stalled, STALLED_INTEGRAL);
    // Load duty
    status->load_duty = duty_due_to_proportional;

    // Calculate duty signal
    duty = duty_due_to_proportional + duty_due_to_integral + duty_due_to_derivative;

    // Check if we are at the target and standing still, with slightly different end conditions for each mode
    if (
        // Conditions for RUN_TIME command
        (
        (maneuver->action == RUN_TIME) &&
            // We are past the total duration of the timed command
            (time_now >= traject->t3)
        )
        ||
        // Conditions for run_stalled commands
        (
        (maneuver->action == RUN_STALLED) &&
            // Whether the motor is stalled in either proportional or integral sense
            *stalled != STALLED_NONE
        )
    )
    {
    // If so, we have reached our goal. We can keep running this loop in order to hold this position.
    // But if brake or coast was specified as the afer_stop, we trigger that. Also clear the running flag to stop waiting for completion.
        if (maneuver->after_stop == PBIO_MOTOR_STOP_COAST) {
            // Coast the motor
            pbio_motor_coast(port);
        }
        else if (maneuver->after_stop == PBIO_MOTOR_STOP_BRAKE) {
            // Brake the motor
            pbio_motor_brake(port);
        }
        else if (maneuver->after_stop == PBIO_MOTOR_STOP_HOLD) {
            // Hold the motor
            // RUN_TIME || RUN_STALLED
            // When ending a time based control maneuver with hold, we trigger a new position based maneuver with zero degrees
            pbio_motor_set_duty_cycle_sys(port, 0);

            pbio_motor_track_target(port, ((float_t) count_now)/settings->counts_per_output_unit);
        }
    }
    // If we are not standing still at a target yet, actuate with the calculated signal
    else {
        pbio_motor_set_duty_cycle_sys(port, duty);
    }
    return PBIO_SUCCESS; // TODO catch and return errors for all above IO tasks
}

void control_update(pbio_port_t port) {
    pbio_error_t err;
    switch (motor[PORT_TO_IDX(port)].state) {
        case PBIO_MOTOR_CONTROL_TRACKING:
        case PBIO_MOTOR_CONTROL_RUNNING_ANGLE:
            err = control_update_angle_target(port);
            break;
        case PBIO_MOTOR_CONTROL_RUNNING_TIME:
            err = control_update_time_target(port);
            break;
        default:
            break;
    }
    if (err != PBIO_SUCCESS) {
        // TODO: process err; set err status to be able to interrupt blocking user wait.
    }

}

#ifdef PBIO_CONFIG_ENABLE_MOTORS
// Service all the motors by calling this function at approximately constant intervals.
void _pbio_motorcontrol_poll(void) {
    // Do the update for each motor
    for (pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++) {
        control_update(port);
    }
}
#endif // PBIO_CONFIG_ENABLE_MOTORS


pbio_error_t pbio_motor_get_initial_state(pbio_port_t port, count_t *count_start, rate_t *rate_start) {
    // TODO If already running, start from ref + set flag of original state

    pbio_error_t err;

    err = pbio_motor_get_encoder_count(port, count_start);
    if (err != PBIO_SUCCESS) { return err; }

    err = pbio_motor_get_encoder_rate(port, rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

void control_init_angle_target(pbio_port_t port) {
    // TODO If already running, start from ref + set flag of original state
    // pbio_motor_control_state_t old_control_mode = motor[PORT_TO_IDX(port)].state;
    pbio_motor_angular_control_status_t *status = &motor[PORT_TO_IDX(port)].angular_control_status;
    pbio_motor_trajectory_t *trajectory = &motor[PORT_TO_IDX(port)].maneuver.trajectory;

    // For this new maneuver, we reset PID variables and related persistent control settings
    // If still running and restarting a new maneuver, however, we keep part of the PID status
    // in order to create a smooth transition from one maneuver to the next.
    // If no previous maneuver was active, just set these to zero.
    status->load_duty = 0;
    status->err_integral = 0;
    status->speed_integrator = 0;
    status->time_paused = 0;
    status->time_stopped = 0;
    status->time_prev = trajectory->t0;
    status->count_err_prev = 0;
    status->windup_status = TIME_RUNNING;
    motor[PORT_TO_IDX(port)].state = PBIO_MOTOR_CONTROL_RUNNING_ANGLE;
}


void control_init_time_target(pbio_port_t port) {
    // TODO If already running, start from ref + set flag of original state
    // pbio_motor_control_state_t old_control_mode = motor[PORT_TO_IDX(port)].state;
    pbio_motor_timed_control_status_t *status = &motor[PORT_TO_IDX(port)].timed_control_status;
    pbio_motor_trajectory_t *trajectory = &motor[PORT_TO_IDX(port)].maneuver.trajectory;

    // Init control depending on old control mode. For now assume old mode was passive, so start from zero,
    status->load_duty = 0;
    status->speed_integrator = 0;
    status->integrator_time_stopped = 0;
    status->windup_status = SPEED_INTEGRATOR_RUNNING;
    status->integrator_start = trajectory->th0;
    status->integrator_ref_start = trajectory->th0;
    motor[PORT_TO_IDX(port)].state = PBIO_MOTOR_CONTROL_RUNNING_TIME;
}


/* pbio user functions */

pbio_error_t pbio_motor_is_stalled(pbio_port_t port, bool *stalled) {
    *stalled = motor[PORT_TO_IDX(port)].stalled != STALLED_NONE &&
               motor[PORT_TO_IDX(port)].state >= PBIO_MOTOR_CONTROL_TRACKING;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run(pbio_port_t port, int32_t speed) {

    // Load motor settings and status
    pbio_motor_settings_t *settings = &motor[PORT_TO_IDX(port)].settings;
    pbio_motor_maneuver_t *maneuver = &motor[PORT_TO_IDX(port)].maneuver;
    pbio_error_t err;

    // Set new maneuver action and stop type
    maneuver->action = RUN;
    maneuver->after_stop = PBIO_MOTOR_STOP_COAST;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    err = pbio_motor_get_initial_state(port, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based_forever(
        time_start,
        count_start,
        rate_start,
        speed * settings->counts_per_output_unit,
        settings->max_rate,
        settings->abs_acceleration,
        &maneuver->trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(port);

    // Run one control update synchronously with user command.
    err = control_update_time_target(port);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop) {
    int32_t angle_now;
    pbio_error_t err;
    switch (after_stop) {
        case PBIO_MOTOR_STOP_COAST:
            // Stop by coasting
            return pbio_motor_coast(port);
        case PBIO_MOTOR_STOP_BRAKE:
            // Stop by braking
            return pbio_motor_brake(port);
        case PBIO_MOTOR_STOP_HOLD:
            // Force stop by holding the current position.
            // First, read where this position is
            err = pbio_motor_get_angle(port, &angle_now);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            // Holding is equivalent to driving to that position actively,
            // which automatically corrects the overshoot that is inevitable
            // when the user requests an immediate stop.
            pbio_motor_track_target(port, angle_now);
        default:
            return PBIO_ERROR_INVALID_ARG;
    }
}

pbio_error_t pbio_motor_run_time(pbio_port_t port, int32_t speed, int32_t duration, pbio_motor_after_stop_t after_stop) {

    // Load motor settings and status
    pbio_motor_settings_t *settings = &motor[PORT_TO_IDX(port)].settings;
    pbio_motor_maneuver_t *maneuver = &motor[PORT_TO_IDX(port)].maneuver;
    pbio_error_t err;

    // Set new maneuver action and stop type
    maneuver->action = RUN_TIME;
    maneuver->after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    err = pbio_motor_get_initial_state(port, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based(
        time_start,
        time_start + duration*US_PER_MS,
        count_start,
        rate_start,
        speed * settings->counts_per_output_unit,
        settings->max_rate,
        settings->abs_acceleration,
        &maneuver->trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(port);

    // Run one control update synchronously with user command.
    err = control_update_time_target(port);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_until_stalled(pbio_port_t port, int32_t speed, pbio_motor_after_stop_t after_stop) {

    // Load motor settings and status
    pbio_motor_settings_t *settings = &motor[PORT_TO_IDX(port)].settings;
    pbio_motor_maneuver_t *maneuver = &motor[PORT_TO_IDX(port)].maneuver;
    pbio_error_t err;

    // Set new maneuver action and stop type
    maneuver->action = RUN_STALLED;
    maneuver->after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    err = pbio_motor_get_initial_state(port, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based_forever(
        time_start,
        count_start,
        rate_start,
        speed * settings->counts_per_output_unit,
        settings->max_rate,
        settings->abs_acceleration,
        &maneuver->trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(port);

    // Run one control update synchronously with user command.
    err = control_update_time_target(port);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_target(pbio_port_t port, int32_t speed, int32_t target, pbio_motor_after_stop_t after_stop) {

    // Load motor settings and status
    pbio_motor_settings_t *settings = &motor[PORT_TO_IDX(port)].settings;
    pbio_motor_maneuver_t *maneuver = &motor[PORT_TO_IDX(port)].maneuver;
    pbio_error_t err;

    // Set new maneuver action and stop type
    maneuver->action = RUN_TARGET;
    maneuver->after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    err = pbio_motor_get_initial_state(port, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_angle_based(
        time_start,
        count_start,
        target*settings->counts_per_output_unit,
        rate_start,
        speed*settings->counts_per_output_unit,
        settings->max_rate,
        settings->abs_acceleration,
        &maneuver->trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(port);

    // Run one control update synchronously with user command.
    err = control_update_angle_target(port);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_angle(pbio_port_t port, int32_t speed, int32_t angle, pbio_motor_after_stop_t after_stop) {

    // Speed  | Angle | End target  | Effect
    //  > 0   |  > 0  | now + angle | Forward
    //  > 0   |  < 0  | now + angle | Backward
    //  < 0   |  > 0  | now - angle | Backward
    //  < 0   |  < 0  | now - angle | Forward

    // Read the instantaneous angle
    int32_t angle_now;
    pbio_error_t err = pbio_motor_get_angle(port, &angle_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // The angle target is the instantaneous angle plus the angle to be traveled
    int32_t angle_target = angle_now + (speed < 0 ? -angle: angle);

    return pbio_motor_run_target(port, speed, angle_target, after_stop);
}

pbio_error_t pbio_motor_track_target(pbio_port_t port, int32_t target) {
    // Load motor settings and status
    pbio_motor_settings_t *settings = &motor[PORT_TO_IDX(port)].settings;
    pbio_motor_maneuver_t *maneuver = &motor[PORT_TO_IDX(port)].maneuver;
    pbio_error_t err;

    // Set new maneuver action and stop type
    maneuver->action = TRACK_TARGET;
    maneuver->after_stop = PBIO_MOTOR_STOP_COAST;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    err = pbio_motor_get_initial_state(port, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    make_trajectory_none(time_start, target*settings->counts_per_output_unit, 0, &maneuver->trajectory);

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(port);

    motor[PORT_TO_IDX(port)].state = PBIO_MOTOR_CONTROL_TRACKING;

    // Run one control update synchronously with user command 
    err = control_update_angle_target(port);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}
