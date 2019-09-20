// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/counter.h>
#include <pbdrv/motor.h>
#include <pbio/servo.h>

#include "sys/clock.h"

static inline int32_t int_fix16_div(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_div(fix16_from_int(a), b));
}

static inline int32_t int_fix16_mul(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_mul(fix16_from_int(a), b));
}

static pbio_servo_t motor[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_servo_get(pbio_port_t port, pbio_servo_t **mtr) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    *mtr = &motor[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    return PBIO_SUCCESS;
}

pbio_error_t pbio_dc_coast(pbio_servo_t *mtr){
    mtr->state = PBIO_CONTROL_COASTING;
    return pbdrv_motor_coast(mtr->port);
}

pbio_error_t pbio_dc_brake(pbio_servo_t *mtr){
    mtr->state = PBIO_CONTROL_BRAKING;
    return pbdrv_motor_set_duty_cycle(mtr->port, 0);
}

pbio_error_t pbio_dc_set_duty_cycle_sys(pbio_servo_t *mtr, int32_t duty_steps) {
    // Limit the duty cycle value
    int32_t limit = mtr->dc.max_duty_steps;
    if (duty_steps > limit) {
        duty_steps = limit;
    }
    if (duty_steps < -limit) {
        duty_steps = -limit;
    }
    int32_t duty_cycle;
    // Add the configured offset and scale remaining duty
    if (duty_steps == 0) {
        duty_cycle = 0;
    }
    else {
        int32_t offset = mtr->dc.duty_offset;
        int32_t offset_signed = duty_steps > 0 ? offset : -offset;
        duty_cycle = offset_signed + ((PBIO_DUTY_STEPS-offset)*duty_steps)/PBIO_DUTY_STEPS;
    }
    // Flip sign if motor is inverted
    if (mtr->dc.direction == PBIO_DIRECTION_COUNTERCLOCKWISE){
        duty_cycle = -duty_cycle;
    }
    return pbdrv_motor_set_duty_cycle(mtr->port, duty_cycle);
}

pbio_error_t pbio_dc_set_duty_cycle_usr(pbio_servo_t *mtr, int32_t duty_steps) {
    mtr->state = PBIO_CONTROL_USRDUTY;
    return pbio_dc_set_duty_cycle_sys(mtr, PBIO_DUTY_STEPS * duty_steps / PBIO_DUTY_USER_STEPS);
}

pbio_error_t pbio_servo_setup(pbio_servo_t *mtr, pbio_direction_t direction, fix16_t gear_ratio) {
    // Coast DC Motor
    pbio_error_t err = pbio_dc_coast(mtr);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    mtr->dc.direction = direction;
    mtr->tacho.direction = direction;
    
    // Reset encoder, and in the process check that it is an encoded motor
    err = pbio_tacho_reset_count(mtr, 0);

    if (err == PBIO_SUCCESS) {
        mtr->has_encoders = true;
    }
    else if (err == PBIO_ERROR_NOT_SUPPORTED) {
        // In this case there are no encoders and we are done by configuring DC settings only
        mtr->has_encoders = false;
        return pbio_dc_set_settings(mtr, 100, 0);
    } else {
        return err;
    }

    // Return on invalid gear ratio
    if (gear_ratio <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    pbio_iodev_type_id_t id;
    err = pbdrv_motor_get_id(mtr->port, &id);
    if (err != PBIO_SUCCESS) { return err; }

    //
    // TODO: Get this ratio from platform config
    //
    fix16_t counts_per_unit;
    if (id == PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR || id == PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR) {
        counts_per_unit = F16C(2, 0);
    }
    else {
        counts_per_unit = F16C(1, 0);
    }

    // Overall ratio between encoder counts and output including gear train
    fix16_t ratio = fix16_mul(counts_per_unit, gear_ratio);

    mtr->tacho.counts_per_unit = counts_per_unit;
    mtr->tacho.counts_per_output_unit = ratio;

    // TODO: Load data by ID rather than hardcoding here, and define shared defaults to reduce size
    if (id == PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR) {
        err = pbio_dc_set_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_run_settings(mtr, int_fix16_div(1200, ratio), int_fix16_div(2400, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_pid_settings(mtr, 400, 600, 5, 100, 3, 5, 2, 200);
        if (err != PBIO_SUCCESS) { return err; }
    }
    else if (id == PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR) {
        err = pbio_dc_set_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_run_settings(mtr, int_fix16_div(800, ratio), int_fix16_div(1600, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_pid_settings(mtr, 500, 800, 5, 100, 3, 5, 2, 200);
        if (err != PBIO_SUCCESS) { return err; }
    }
    else if (id == PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR) {
        err = pbio_dc_set_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_run_settings(mtr, int_fix16_div(1500, ratio), int_fix16_div(3000, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_pid_settings(mtr, 400, 600, 5, 100, 3, 5, 2, 200);
        if (err != PBIO_SUCCESS) { return err; }
    }
    else {
        // Defaults
        err = pbio_dc_set_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_run_settings(mtr, int_fix16_div(1000, ratio), int_fix16_div(1000, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_servo_set_pid_settings(mtr, 500, 800, 5, 100, 3, 5, 2, 500);
        if (err != PBIO_SUCCESS) { return err; }
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_dc_set_settings(pbio_servo_t *mtr, int32_t stall_torque_limit_pct, int32_t duty_offset_pct) {
    if (stall_torque_limit_pct < 0 || duty_offset_pct < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    mtr->dc.max_duty_steps = PBIO_DUTY_STEPS_PER_USER_STEP * stall_torque_limit_pct;
    mtr->dc.duty_offset = PBIO_DUTY_STEPS_PER_USER_STEP * duty_offset_pct;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_dc_get_settings(pbio_servo_t *mtr, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct) {
    *stall_torque_limit_pct = mtr->dc.max_duty_steps/PBIO_DUTY_STEPS_PER_USER_STEP;
    *duty_offset_pct = mtr->dc.duty_offset/PBIO_DUTY_STEPS_PER_USER_STEP;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_set_run_settings(pbio_servo_t *mtr, int32_t max_speed, int32_t acceleration) {
    mtr->control.settings.max_rate = int_fix16_mul(max_speed, mtr->tacho.counts_per_output_unit);
    mtr->control.settings.abs_acceleration = int_fix16_mul(acceleration, mtr->tacho.counts_per_output_unit);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_set_pid_settings(pbio_servo_t *mtr,
                                         int16_t pid_kp,
                                         int16_t pid_ki,
                                         int16_t pid_kd,
                                         int32_t tight_loop_time,
                                         int32_t position_tolerance,
                                         int32_t speed_tolerance,
                                         int32_t stall_speed_limit,
                                         int32_t stall_time) {
    fix16_t counts_per_output_unit = mtr->tacho.counts_per_output_unit;

    if (pid_kp < 0 || pid_ki < 0 || pid_kd < 0 || tight_loop_time < 0 ||
        position_tolerance < 0 || speed_tolerance < 0 || stall_speed_limit < 0 || stall_time < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    mtr->control.settings.pid_kp = pid_kp;
    mtr->control.settings.pid_ki = pid_ki;
    mtr->control.settings.pid_kd = pid_kd;
    mtr->control.settings.tight_loop_time = tight_loop_time * US_PER_MS;
    mtr->control.settings.count_tolerance = int_fix16_mul(position_tolerance, counts_per_output_unit);
    mtr->control.settings.rate_tolerance = int_fix16_mul(speed_tolerance, counts_per_output_unit);
    mtr->control.settings.stall_rate_limit = int_fix16_mul(stall_speed_limit, counts_per_output_unit);
    mtr->control.settings.stall_time = stall_time * US_PER_MS;
    return PBIO_SUCCESS;
}

void pbio_servo_print_settings(pbio_servo_t *mtr, char *dc_settings_string, char *enc_settings_string) {
    char *direction = mtr->dc.direction == PBIO_DIRECTION_CLOCKWISE ? "clockwise" : "counterclockwise";
    snprintf(dc_settings_string, MAX_DCMOTOR_SETTINGS_STR_LENGTH,
        "Motor properties:\n"
        "------------------------\n"
        "Port\t\t %c\n"
        "Direction\t %s",
        mtr->port,
        direction
    );
    if (!pbio_motor_has_encoder(mtr)) {
        enc_settings_string[0] = 0;
    }
    else {
        char counts_per_unit_str[13];
        char gear_ratio_str[13];
        // Preload several settings for easier printing
        fix16_t counts_per_output_unit = mtr->tacho.counts_per_output_unit;
        fix16_t counts_per_unit = mtr->tacho.counts_per_unit;
        fix16_t gear_ratio = fix16_div(counts_per_output_unit, mtr->tacho.counts_per_unit);
        fix16_to_str(counts_per_unit, counts_per_unit_str, 3);
        fix16_to_str(gear_ratio, gear_ratio_str, 3);
        // Print settings to settings_string
        snprintf(enc_settings_string, MAX_ENCMOTOR_SETTINGS_STR_LENGTH,
            "Counts per unit\t %s\n"
            "Gear ratio\t %s\n"
            "\nRun settings:\n"
            "------------------------\n"
            "Max speed\t %" PRId32 "\n"
            "Acceleration\t %" PRId32 "\n"
            "\nDC settings:\n"
            "------------------------\n"
            "Duty limit\t %" PRId32 "\n"
            "Duty offset\t %" PRId32 "\n"
            "\nPID settings:\n"
            "------------------------\n"
            "kp\t\t %" PRId32 "\n"
            "ki\t\t %" PRId32 "\n"
            "kd\t\t %" PRId32 "\n"
            "Tight Loop\t %" PRId32 "\n"
            "Angle tolerance\t %" PRId32 "\n"
            "Speed tolerance\t %" PRId32 "\n"
            "Stall speed\t %" PRId32 "\n"
            "Stall time\t %" PRId32,
            counts_per_unit_str,
            gear_ratio_str,
            // Print run settings
            int_fix16_div(mtr->control.settings.max_rate, counts_per_output_unit),
            int_fix16_div(mtr->control.settings.abs_acceleration, counts_per_output_unit),
            // Print DC settings
            (int32_t) (mtr->dc.max_duty_steps / PBIO_DUTY_STEPS_PER_USER_STEP),
            (int32_t) (mtr->dc.duty_offset / PBIO_DUTY_STEPS_PER_USER_STEP),
            // Print PID settings
            (int32_t) mtr->control.settings.pid_kp,
            (int32_t) mtr->control.settings.pid_ki,
            (int32_t) mtr->control.settings.pid_kd,
            (int32_t) (mtr->control.settings.tight_loop_time / US_PER_MS),
            int_fix16_div(mtr->control.settings.count_tolerance, counts_per_output_unit),
            int_fix16_div(mtr->control.settings.rate_tolerance, counts_per_output_unit),
            int_fix16_div(mtr->control.settings.stall_rate_limit, counts_per_output_unit),
            (int32_t) (mtr->control.settings.stall_time  / US_PER_MS)
        );
    }
}

bool pbio_motor_has_encoder(pbio_servo_t *mtr) {
    return mtr->has_encoders;
}

pbio_error_t pbio_tacho_get_count(pbio_servo_t *mtr, int32_t *count) {
    pbdrv_counter_dev_t *tacho_counter;
    pbio_error_t err;

    // TODO: get tacho_counter once at init when this is converted to contiki process
    err = pbdrv_counter_get(mtr->counter_id, &tacho_counter);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = pbdrv_counter_get_count(tacho_counter, count);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (mtr->tacho.direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        *count = -*count;
    }
    *count -= mtr->tacho.offset;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_tacho_reset_count(pbio_servo_t *mtr, int32_t reset_count) {
    int32_t count_no_offset;
    pbio_error_t err;

    // First get the counter value without any offsets, but with the appropriate polarity/sign.
    err = pbio_tacho_get_count(mtr, &count_no_offset);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    count_no_offset += mtr->tacho.offset;

    // Calculate the new offset
    mtr->tacho.offset = count_no_offset - reset_count;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_tacho_get_angle(pbio_servo_t *mtr, int32_t *angle) {
    int32_t encoder_count;
    pbio_error_t err;

    err = pbio_tacho_get_count(mtr, &encoder_count);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *angle = int_fix16_div(encoder_count, mtr->tacho.counts_per_output_unit);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_tacho_reset_angle(pbio_servo_t *mtr, int32_t reset_angle) {
    // Load motor settings and status
    pbio_error_t err;

    // Perform angle reset in case of tracking / holding
    if (mtr->state == PBIO_CONTROL_ANGLE_BACKGROUND && mtr->control.action == TRACK_TARGET) {
        // Get the old angle
        int32_t angle_old;
        err = pbio_tacho_get_count(mtr, &angle_old);
        if (err != PBIO_SUCCESS) { return err; }
        // Get the old target
        int32_t target_old = int_fix16_div(mtr->control.trajectory.th3, mtr->tacho.counts_per_output_unit);
        // Reset the angle
        err = pbio_tacho_reset_count(mtr, int_fix16_mul(reset_angle, mtr->tacho.counts_per_output_unit));
        if (err != PBIO_SUCCESS) { return err; }
        // Set the new target based on the old angle and the old target, after the angle reset
        int32_t new_target = reset_angle + target_old - angle_old;
        return pbio_servo_track_target(mtr, new_target);

    }
    // If the motor was in a passive mode (coast, brake, user duty), reset angle and leave state unchanged
    else if (mtr->state <= PBIO_CONTROL_USRDUTY){
        return pbio_tacho_reset_count(mtr, int_fix16_mul(reset_angle, mtr->tacho.counts_per_output_unit));
    }
    // In all other cases, stop the ongoing maneuver by coasting and then reset the angle
    else {
        err = pbio_dc_coast(mtr);
        if (err != PBIO_SUCCESS) { return err; }
        return pbio_tacho_reset_count(mtr, int_fix16_mul(reset_angle, mtr->tacho.counts_per_output_unit));
    }
}

pbio_error_t pbio_tacho_get_rate(pbio_servo_t *mtr, int32_t *rate) {
    pbdrv_counter_dev_t *tacho_counter;
    pbio_error_t err;

    // TODO: get tacho_counter once at init when this is converted to contiki process
    err = pbdrv_counter_get(mtr->counter_id, &tacho_counter);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = pbdrv_counter_get_rate(tacho_counter, rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    if (mtr->tacho.direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        *rate = -*rate;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_tacho_get_angular_rate(pbio_servo_t *mtr, int32_t *angular_rate) {
    int32_t encoder_rate;
    pbio_error_t err;

    err = pbio_tacho_get_rate(mtr, &encoder_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *angular_rate = int_fix16_div(encoder_rate, mtr->tacho.counts_per_output_unit);

    return PBIO_SUCCESS;
}

// Get the physical state of a single motor
static pbio_error_t control_get_state(pbio_servo_t *mtr, ustime_t *time_now, count_t *count_now, rate_t *rate_now) {

    pbio_error_t err;

    // Read current state of this motor: current time, speed, and position
    *time_now = clock_usecs();
    err = pbio_tacho_get_count(mtr, count_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_tacho_get_rate(mtr, rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_SUCCESS;
}

// Actuate a single motor
static pbio_error_t control_update_actuate(pbio_servo_t *mtr, pbio_control_after_stop_t actuation_type, int32_t control) {

    pbio_error_t err = PBIO_SUCCESS;

    // Apply the calculated actuation, by type
    switch (actuation_type)
    {
    case PBIO_MOTOR_STOP_COAST:
        err = pbio_dc_coast(mtr);
        break;
    case PBIO_MOTOR_STOP_BRAKE:
        err = pbio_dc_brake(mtr);
        break;
    case PBIO_MOTOR_STOP_HOLD:
        err = pbio_servo_track_target(mtr, int_fix16_div(control, mtr->tacho.counts_per_output_unit));
        break;
    case PBIO_ACTUATION_DUTY:
        err = pbio_dc_set_duty_cycle_sys(mtr, control);
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

pbio_error_t pbio_servo_control_update(pbio_servo_t *mtr) {

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

static pbio_error_t pbio_motor_get_initial_state(pbio_servo_t *mtr, count_t *count_start, rate_t *rate_start) {

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
        err = pbio_tacho_get_count(mtr, count_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        err = pbio_tacho_get_rate(mtr, rate_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    return PBIO_SUCCESS;
}

/* pbio user functions */

pbio_error_t pbio_servo_is_stalled(pbio_servo_t *mtr, bool *stalled) {
    *stalled = mtr->control.stalled > STALLED_NONE &&
               mtr->state >= PBIO_CONTROL_ANGLE_BACKGROUND;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run(pbio_servo_t *mtr, int32_t speed) {
    if (mtr->state == PBIO_CONTROL_TIME_BACKGROUND &&
        mtr->control.action == RUN &&
        int_fix16_mul(speed, mtr->tacho.counts_per_output_unit) == mtr->control.trajectory.w1) {
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
        int_fix16_mul(speed, mtr->tacho.counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(mtr);

    // Run is always in the background
    mtr->state = PBIO_CONTROL_TIME_BACKGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_stop(pbio_servo_t *mtr, pbio_control_after_stop_t after_stop) {
    int32_t angle_now;
    pbio_error_t err;
    switch (after_stop) {
        case PBIO_MOTOR_STOP_COAST:
            // Stop by coasting
            return pbio_dc_coast(mtr);
        case PBIO_MOTOR_STOP_BRAKE:
            // Stop by braking
            return pbio_dc_brake(mtr);
        case PBIO_MOTOR_STOP_HOLD:
            // Force stop by holding the current position.
            // First, read where this position is
            err = pbio_tacho_get_angle(mtr, &angle_now);
            if (err != PBIO_SUCCESS) { return err; }
            // Holding is equivalent to driving to that position actively,
            // which automatically corrects the overshoot that is inevitable
            // when the user requests an immediate stop.
            return pbio_servo_track_target(mtr, angle_now);
        default:
            return PBIO_ERROR_INVALID_ARG;
    }
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *mtr, int32_t speed, int32_t duration, pbio_control_after_stop_t after_stop, bool foreground) {
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
        int_fix16_mul(speed, mtr->tacho.counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(mtr);

    // Set user specified foreground or background state
    mtr->state = foreground ? PBIO_CONTROL_TIME_FOREGROUND : PBIO_CONTROL_TIME_BACKGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(mtr);

    return err;
}

pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *mtr, int32_t speed, pbio_control_after_stop_t after_stop) {
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
        int_fix16_mul(speed, mtr->tacho.counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(mtr);

    // Run until stalled is always in the foreground
    mtr->state = PBIO_CONTROL_TIME_FOREGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run_target(pbio_servo_t *mtr, int32_t speed, int32_t target, pbio_control_after_stop_t after_stop, bool foreground) {
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
        int_fix16_mul(target, mtr->tacho.counts_per_output_unit),
        rate_start,
        int_fix16_mul(speed, mtr->tacho.counts_per_output_unit),
        mtr->control.settings.max_rate,
        mtr->control.settings.abs_acceleration,
        &mtr->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(mtr);

    // Set user specified foreground or background state
    mtr->state = foreground ? PBIO_CONTROL_ANGLE_FOREGROUND : PBIO_CONTROL_ANGLE_BACKGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run_angle(pbio_servo_t *mtr, int32_t speed, int32_t angle, pbio_control_after_stop_t after_stop, bool foreground) {

    // Speed  | Angle | End target  | Effect
    //  > 0   |  > 0  | now + angle | Forward
    //  > 0   |  < 0  | now + angle | Backward
    //  < 0   |  > 0  | now - angle | Backward
    //  < 0   |  < 0  | now - angle | Forward

    // Read the instantaneous angle
    int32_t angle_now;
    pbio_error_t err = pbio_tacho_get_angle(mtr, &angle_now);
    if (err != PBIO_SUCCESS) { return err; }

    // The angle target is the instantaneous angle plus the angle to be traveled
    int32_t angle_target = angle_now + (speed < 0 ? -angle: angle);

    return pbio_servo_run_target(mtr, speed, angle_target, after_stop, foreground);
}

// generalize to generic control? or sub call that is generic? then hold after generic timed can use it too

// then we can also specify target as counts to avoid duplicate scaling, i.e. when we call hold from timed

pbio_error_t pbio_servo_track_target(pbio_servo_t *mtr, int32_t target) {
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
    make_trajectory_none(time_start, int_fix16_mul(target, mtr->tacho.counts_per_output_unit), 0, &mtr->control.trajectory);

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(mtr);

    // Tracking a target is always a background action
    mtr->state = PBIO_CONTROL_ANGLE_BACKGROUND;

    // Run one control update synchronously with user command
    err = pbio_servo_control_update(mtr);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

// TODO: convert these two functions to contiki process
void _pbio_servo_init(void) {
#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
    int i;

    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        motor[i].port = PBDRV_CONFIG_FIRST_MOTOR_PORT + i;
        motor[i].counter_id = i;
    }
#endif
}

// Service all the motors by calling this function at approximately constant intervals.
void _pbio_servo_poll(void) {
    int i;

    // Do the update for each motor
    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbio_servo_control_update(&motor[i]);
    }
}
