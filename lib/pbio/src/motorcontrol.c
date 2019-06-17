// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <fixmath.h>

#include <pbdrv/counter.h>
#include <pbio/motor.h>
#include <pbio/motorref.h>

#include "sys/clock.h"

static pbio_motor_t motor[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

// TODO: this function belongs in motor.c, but we need to separate the motor
// from the controller first
pbio_error_t pbio_motor_get(uint8_t index, pbio_motor_t **mtr) {
    if (index >= PBDRV_CONFIG_NUM_MOTOR_CONTROLLER) {
        return PBIO_ERROR_INVALID_ARG;
    }
    *mtr = &motor[index];
    return PBIO_SUCCESS;
}

static inline int32_t int_fix16_div(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_div(fix16_from_int(a), b));
}

static inline int32_t int_fix16_mul(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_mul(fix16_from_int(a), b));
}

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

static pbio_error_t control_update_angle_target(pbio_motor_t *mtr) {
    pbdrv_counter_dev_t *tacho_counter;
    pbio_error_t err;

    // TODO: get tacho_counter once at init when this is converted to contiki process
    err = pbdrv_counter_get(mtr->counter_id, &tacho_counter);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Trajectory and setting shortcuts for this motor
    pbio_control_status_angular_t *status = &mtr->control.status_angular;
    duty_t max_duty = mtr->max_duty_steps;

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_now, time_ref, time_loop;
    count_t count_now, count_ref, count_err;
    rate_t rate_now, rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Read current state of this motor: current time, speed, and position
    time_now = clock_usecs();
    err = pbdrv_counter_get_count(tacho_counter, &count_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = pbdrv_counter_get_rate(tacho_counter, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

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
    // If so, we have reached our goal. We can keep running this loop in order to hold this position.
    // But if brake or coast was specified as the afer_stop, we trigger that. Also clear the running flag to stop waiting for completion.
        if (mtr->control.after_stop == PBIO_MOTOR_STOP_COAST) {
            // Coast the motor
            err = pbio_motor_coast(mtr);
            if (err != PBIO_SUCCESS) { return err; }
        }
        else if (mtr->control.after_stop == PBIO_MOTOR_STOP_BRAKE) {
            // Brake the motor
            err = pbio_motor_brake(mtr);
            if (err != PBIO_SUCCESS) { return err; }
        }
        else if (mtr->control.after_stop == PBIO_MOTOR_STOP_HOLD) {
            // Hold the motor. In position based control, holding just means that we continue the position control loop without changes
            err = pbio_motor_set_duty_cycle_sys(mtr, duty);
            if (err != PBIO_SUCCESS) { return err; }

            // Altough we keep holding, the maneuver is completed
            mtr->state = PBIO_CONTROL_ANGLE_BACKGROUND;

        }
    }
    // If we are not standing still at a target yet, actuate with the calculated signal
    else {
        err = pbio_motor_set_duty_cycle_sys(mtr, duty);
        if (err != PBIO_SUCCESS) { return err; }
    }
    return PBIO_SUCCESS;
}

static pbio_error_t control_update_time_target(pbio_motor_t *mtr) {
    pbdrv_counter_dev_t *tacho_counter;
    pbio_error_t err;

    // TODO: get tacho_counter once at init when this is converted to contiki process
    err = pbdrv_counter_get(mtr->counter_id, &tacho_counter);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Trajectory and setting shortcuts for this motor
    pbio_control_status_timed_t *status = &mtr->control.status_timed;
    duty_t max_duty = mtr->max_duty_steps;

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_now;
    count_t count_now, count_ref, count_err;
    rate_t rate_now, rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Read current state of this motor: current time, speed, and position
    time_now = clock_usecs();
    err = pbdrv_counter_get_count(tacho_counter, &count_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbdrv_counter_get_rate(tacho_counter, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

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
    // If so, we have reached our goal and we trigger the stop
        if (mtr->control.after_stop == PBIO_MOTOR_STOP_COAST) {
            // Coast the motor
            err = pbio_motor_coast(mtr);
            if (err != PBIO_SUCCESS) { return err; }
        }
        else if (mtr->control.after_stop == PBIO_MOTOR_STOP_BRAKE) {
            // Brake the motor
            err = pbio_motor_brake(mtr);
            if (err != PBIO_SUCCESS) { return err; }
        }
        else if (mtr->control.after_stop == PBIO_MOTOR_STOP_HOLD) {
            // Hold the motor. When ending a time based control maneuver with hold, we trigger a new position based maneuver with zero degrees
            err = pbio_motor_track_target(mtr, int_fix16_div(count_now, mtr->counts_per_output_unit));
            if (err != PBIO_SUCCESS) { return err; }
        }
    }
    // If we are not standing still at a target yet, actuate with the calculated signal
    else {
        err = pbio_motor_set_duty_cycle_sys(mtr, duty);
        if (err != PBIO_SUCCESS) { return err; }
    }
    return PBIO_SUCCESS;
}

static void control_update(pbio_motor_t *mtr) {
    pbio_error_t err = PBIO_SUCCESS;
    switch (mtr->state) {
        // Update the angular control in these modes
        case PBIO_CONTROL_ANGLE_BACKGROUND:
        case PBIO_CONTROL_ANGLE_FOREGROUND:
            err = control_update_angle_target(mtr);
            break;
        // Update the timed control in these modes
        case PBIO_CONTROL_TIME_BACKGROUND:
        case PBIO_CONTROL_TIME_FOREGROUND:
            err = control_update_time_target(mtr);
            break;
        default:
            break;
    }
    // Process errors raised during control update
    if (err != PBIO_SUCCESS) {
        // Attempt lowest level coast without checking
        // for further errors: turn off power
        pbdrv_motor_coast(mtr->port);

        // Let foreground tasks know about error in order to stop blocking wait tasks
        mtr->state = PBIO_CONTROL_ERRORED;
    }

}

// TODO: convert these two functions to contiki process
void _pbio_motorcontroll_init(void) {
#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
    int i;

    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        motor[i].port = PBDRV_CONFIG_FIRST_MOTOR_PORT + i;
        motor[i].counter_id = i;
    }
#endif
}

// Service all the motors by calling this function at approximately constant intervals.
void _pbio_motorcontrol_poll(void) {
    int i;

    // Do the update for each motor
    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        control_update(&motor[i]);
    }
}

static pbio_error_t pbio_motor_get_initial_state(pbio_motor_t *mtr, count_t *count_start, rate_t *rate_start) {
    pbdrv_counter_dev_t *tacho_counter;
    ustime_t time_now = clock_usecs();
    pbio_error_t err;

    // TODO: get tacho_counter once at init when this is converted to contiki process
    err = pbdrv_counter_get(mtr->counter_id, &tacho_counter);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (mtr->state == PBIO_CONTROL_TIME_FOREGROUND || mtr->state == PBIO_CONTROL_TIME_BACKGROUND) {
        get_reference(time_now, &mtr->control.trajectory, count_start, rate_start);
    }
    else if (mtr->state == PBIO_CONTROL_ANGLE_FOREGROUND || mtr->state == PBIO_CONTROL_ANGLE_BACKGROUND) {
        pbio_control_status_angular_t status = mtr->control.status_angular;
        ustime_t time_ref = status.ref_time_running ?
            time_now - status.time_paused :
            status.time_stopped - status.time_paused;
        get_reference(time_ref, &mtr->control.trajectory, count_start, rate_start);
    }
    else {
        // Otherwise, we are not currently in a control mode, and we start from the instantaneous motor state
        err = pbdrv_counter_get_count(tacho_counter, count_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        err = pbdrv_counter_get_rate(tacho_counter, rate_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    return PBIO_SUCCESS;
}

static void control_init_angle_target(pbio_motor_t *mtr) {
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


static void control_init_time_target(pbio_motor_t *mtr) {
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


/* pbio user functions */

pbio_error_t pbio_motor_is_stalled(pbio_motor_t *mtr, bool *stalled) {
    *stalled = mtr->control.stalled > STALLED_NONE &&
               mtr->state >= PBIO_CONTROL_ANGLE_BACKGROUND;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run(pbio_motor_t *mtr, int32_t speed) {
    if (mtr->state == PBIO_CONTROL_TIME_BACKGROUND &&
        mtr->control.action == RUN &&
        int_fix16_mul(speed, mtr->counts_per_output_unit) == mtr->control.trajectory.w1) {
        // If the exact same command is already running, there is nothing we need to do
        return PBIO_SUCCESS;
    }

    // Set new maneuver action and stop type
    mtr->control.action = RUN;
    mtr->control.after_stop = PBIO_MOTOR_STOP_COAST;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(mtr, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based_forever(
        time_start,
        count_start,
        rate_start,
        int_fix16_mul(speed, mtr->counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(mtr);

    // Run one control update synchronously with user command.
    err = control_update_time_target(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    // Run is always in the background
    mtr->state = PBIO_CONTROL_TIME_BACKGROUND;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_stop(pbio_motor_t *mtr, pbio_control_after_stop_t after_stop) {
    int32_t angle_now;
    pbio_error_t err;
    switch (after_stop) {
        case PBIO_MOTOR_STOP_COAST:
            // Stop by coasting
            return pbio_motor_coast(mtr);
        case PBIO_MOTOR_STOP_BRAKE:
            // Stop by braking
            return pbio_motor_brake(mtr);
        case PBIO_MOTOR_STOP_HOLD:
            // Force stop by holding the current position.
            // First, read where this position is
            err = pbio_motor_get_angle(mtr, &angle_now);
            if (err != PBIO_SUCCESS) { return err; }
            // Holding is equivalent to driving to that position actively,
            // which automatically corrects the overshoot that is inevitable
            // when the user requests an immediate stop.
            return pbio_motor_track_target(mtr, angle_now);
        default:
            return PBIO_ERROR_INVALID_ARG;
    }
}

pbio_error_t pbio_motor_run_time(pbio_motor_t *mtr, int32_t speed, int32_t duration, pbio_control_after_stop_t after_stop, bool foreground) {
    // Set new maneuver action and stop type
    mtr->control.action = RUN_TIME;
    mtr->control.after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(mtr, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based(
        time_start,
        time_start + duration*US_PER_MS,
        count_start,
        rate_start,
        int_fix16_mul(speed, mtr->counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(mtr);

    // Run one control update synchronously with user command.
    err = control_update_time_target(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    // Set user specified foreground or background state
    mtr->state = foreground ? PBIO_CONTROL_TIME_FOREGROUND : PBIO_CONTROL_TIME_BACKGROUND;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_until_stalled(pbio_motor_t *mtr, int32_t speed, pbio_control_after_stop_t after_stop) {
    // Set new maneuver action and stop type
    mtr->control.action = RUN_STALLED;
    mtr->control.after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(mtr, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based_forever(
        time_start,
        count_start,
        rate_start,
        int_fix16_mul(speed, mtr->counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(mtr);

    // Run one control update synchronously with user command.
    err = control_update_time_target(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    // Run until stalled is always in the foreground
    mtr->state = PBIO_CONTROL_TIME_FOREGROUND;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_target(pbio_motor_t *mtr, int32_t speed, int32_t target, pbio_control_after_stop_t after_stop, bool foreground) {
    // Set new maneuver action and stop type
    mtr->control.action = RUN_TARGET;
    mtr->control.after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(mtr, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_angle_based(
        time_start,
        count_start,
        int_fix16_mul(target, mtr->counts_per_output_unit),
        rate_start,
        int_fix16_mul(speed, mtr->counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(mtr);

    // Run one control update synchronously with user command.
    err = control_update_angle_target(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    // Set user specified foreground or background state
    mtr->state = foreground ? PBIO_CONTROL_ANGLE_FOREGROUND : PBIO_CONTROL_ANGLE_BACKGROUND;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_run_angle(pbio_motor_t *mtr, int32_t speed, int32_t angle, pbio_control_after_stop_t after_stop, bool foreground) {

    // Speed  | Angle | End target  | Effect
    //  > 0   |  > 0  | now + angle | Forward
    //  > 0   |  < 0  | now + angle | Backward
    //  < 0   |  > 0  | now - angle | Backward
    //  < 0   |  < 0  | now - angle | Forward

    // Read the instantaneous angle
    int32_t angle_now;
    pbio_error_t err = pbio_motor_get_angle(mtr, &angle_now);
    if (err != PBIO_SUCCESS) { return err; }

    // The angle target is the instantaneous angle plus the angle to be traveled
    int32_t angle_target = angle_now + (speed < 0 ? -angle: angle);

    return pbio_motor_run_target(mtr, speed, angle_target, after_stop, foreground);
}

pbio_error_t pbio_motor_track_target(pbio_motor_t *mtr, int32_t target) {
    // Set new maneuver action and stop type
    mtr->control.action = TRACK_TARGET;
    mtr->control.after_stop = PBIO_MOTOR_STOP_COAST;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(mtr, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    make_trajectory_none(time_start, int_fix16_mul(target, mtr->counts_per_output_unit), 0, &mtr->control.trajectory);

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(mtr);

    // Run one control update synchronously with user command
    err = control_update_angle_target(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    // Tracking a target is always a background action
    mtr->state = PBIO_CONTROL_ANGLE_BACKGROUND;

    return PBIO_SUCCESS;
}
