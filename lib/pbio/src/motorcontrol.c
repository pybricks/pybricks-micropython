// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <fixmath.h>

#include <pbio/servo.h>
#include <pbio/trajectory.h>

#include "sys/clock.h"

static inline int32_t int_fix16_div(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_div(fix16_from_int(a), b));
}

static inline int32_t int_fix16_mul(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_mul(fix16_from_int(a), b));
}

// Get the physical state of a single motor
static pbio_error_t control_get_state(pbio_motor_t *mtr, ustime_t *time_now, count_t *count_now, rate_t *rate_now) {

    pbio_error_t err;

    // Read current state of this motor: current time, speed, and position
    *time_now = clock_usecs();
    err = pbio_motor_get_encoder_count(mtr, count_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_motor_get_encoder_rate(mtr, rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_SUCCESS;
}

// Actuate a single motor
static pbio_error_t control_update_actuate(pbio_motor_t *mtr, pbio_control_after_stop_t actuation_type, int32_t control) {

    pbio_error_t err = PBIO_SUCCESS;

    // Apply the calculated actuation, by type
    switch (actuation_type)
    {
    case PBIO_MOTOR_STOP_COAST:
        err = pbio_motor_coast(mtr);
        break;
    case PBIO_MOTOR_STOP_BRAKE:
        err = pbio_motor_brake(mtr);
        break;
    case PBIO_MOTOR_STOP_HOLD:
        err = pbio_motor_track_target(mtr, int_fix16_div(control, mtr->counts_per_output_unit));
        break;
    case PBIO_ACTUATION_DUTY:
        err = pbio_motor_set_duty_cycle_sys(mtr, control);
        break;
    }
    
    // Handle errors during actuation
    if (err != PBIO_SUCCESS) {
        // Attempt lowest level coast: turn off power
        pbdrv_motor_coast(mtr->port);

        // Let foreground tasks know about error in order to stop blocking wait tasks
        mtr->state = PBIO_CONTROL_ERRORED;
    }
    return err;
}

pbio_error_t control_update(pbio_motor_t *mtr) {

    // Passive modes do not need control
    if (mtr->state <= PBIO_CONTROL_ERRORED) {
        return PBIO_SUCCESS;
    }
    // Read the physical state
    ustime_t time_now;
    count_t count_now;
    rate_t rate_now;
    pbio_error_t err = control_get_state(mtr, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Control action to be calculated
    pbio_control_after_stop_t actuation = PBIO_MOTOR_STOP_COAST;
    int32_t control = 0;

    // Calculate controls for position based control
    if (mtr->state == PBIO_CONTROL_ANGLE_BACKGROUND ||
        mtr->state == PBIO_CONTROL_ANGLE_FOREGROUND) {
        err = control_update_angle_target(mtr, time_now, count_now, rate_now, &actuation, &control);
    }
    // Calculate controls for time based control
    else if (mtr->state == PBIO_CONTROL_TIME_BACKGROUND ||
             mtr->state == PBIO_CONTROL_TIME_FOREGROUND) {
        // Get control type and signal for given state
        err = control_update_time_target(mtr, time_now, count_now, rate_now, &actuation, &control);
    }
    if (err != PBIO_SUCCESS) {
        return err;
    } 
    // Apply the control type and signal
    return control_update_actuate(mtr, actuation, control);
}

static pbio_error_t pbio_motor_get_initial_state(pbio_motor_t *mtr, count_t *count_start, rate_t *rate_start) {

    ustime_t time_now = clock_usecs();
    pbio_error_t err;

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
        // TODO: use generic get state functions

        // Otherwise, we are not currently in a control mode, and we start from the instantaneous motor state
        err = pbio_motor_get_encoder_count(mtr, count_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        err = pbio_motor_get_encoder_rate(mtr, rate_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    return PBIO_SUCCESS;
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

    // Run is always in the background
    mtr->state = PBIO_CONTROL_TIME_BACKGROUND;

    // Run one control update synchronously with user command.
    err = control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

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

    // Set user specified foreground or background state
    mtr->state = foreground ? PBIO_CONTROL_TIME_FOREGROUND : PBIO_CONTROL_TIME_BACKGROUND;

    // Run one control update synchronously with user command.
    err = control_update(mtr);

    return err;
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

    // Run until stalled is always in the foreground
    mtr->state = PBIO_CONTROL_TIME_FOREGROUND;

    // Run one control update synchronously with user command.
    err = control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

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

    // Set user specified foreground or background state
    mtr->state = foreground ? PBIO_CONTROL_ANGLE_FOREGROUND : PBIO_CONTROL_ANGLE_BACKGROUND;

    // Run one control update synchronously with user command.
    err = control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

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

// generalize to generic control? or sub call that is generic? then hold after generic timed can use it too

// then we can also specify target as counts to avoid duplicate scaling, i.e. when we call hold from timed

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

    // Tracking a target is always a background action
    mtr->state = PBIO_CONTROL_ANGLE_BACKGROUND;

    // Run one control update synchronously with user command
    err = control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}
