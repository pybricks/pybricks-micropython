// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/counter.h>
#include <pbdrv/motor.h>
#include <pbio/motor.h>

static inline int32_t int_fix16_div(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_div(fix16_from_int(a), b));
}

static inline int32_t int_fix16_mul(int32_t a, fix16_t b) {
    return fix16_to_int(fix16_mul(fix16_from_int(a), b));
}

static pbio_motor_t motor[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_motor_get(pbio_port_t port, pbio_motor_t **mtr) {
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    *mtr = &motor[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_coast(pbio_motor_t *mtr){
    mtr->state = PBIO_CONTROL_COASTING;
    return pbdrv_motor_coast(mtr->port);
}

pbio_error_t pbio_motor_brake(pbio_motor_t *mtr){
    mtr->state = PBIO_CONTROL_BRAKING;
    return pbdrv_motor_set_duty_cycle(mtr->port, 0);
}

pbio_error_t pbio_motor_set_duty_cycle_sys(pbio_motor_t *mtr, int32_t duty_steps) {
    // Limit the duty cycle value
    int32_t limit = mtr->max_duty_steps;
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
        int32_t offset = mtr->duty_offset;
        int32_t offset_signed = duty_steps > 0 ? offset : -offset;
        duty_cycle = offset_signed + ((PBIO_DUTY_STEPS-offset)*duty_steps)/PBIO_DUTY_STEPS;
    }
    // Flip sign if motor is inverted
    if (mtr->direction == PBIO_MOTOR_DIR_COUNTERCLOCKWISE){
        duty_cycle = -duty_cycle;
    }
    return pbdrv_motor_set_duty_cycle(mtr->port, duty_cycle);
}

pbio_error_t pbio_motor_set_duty_cycle_usr(pbio_motor_t *mtr, int32_t duty_steps) {
    mtr->state = PBIO_CONTROL_USRDUTY;
    return pbio_motor_set_duty_cycle_sys(mtr, PBIO_DUTY_STEPS * duty_steps / PBIO_DUTY_USER_STEPS);
}

pbio_error_t pbio_motor_setup(pbio_motor_t *mtr, pbio_motor_dir_t direction, fix16_t gear_ratio) {
    // Coast DC Motor
    pbio_error_t err = pbio_motor_coast(mtr);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    mtr->direction = direction;
    
    // Reset encoder, and in the process check that it is an encoded motor
    err = pbio_motor_reset_encoder_count(mtr, 0);

    if (err == PBIO_SUCCESS) {
        mtr->has_encoders = true;
    }
    else if (err == PBIO_ERROR_NOT_SUPPORTED) {
        // In this case there are no encoders and we are done by configuring DC settings only
        mtr->has_encoders = false;
        return pbio_motor_set_dc_settings(mtr, 100, 0);
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

    mtr->counts_per_unit = counts_per_unit;
    mtr->counts_per_output_unit = ratio;

    // TODO: Load data by ID rather than hardcoding here, and define shared defaults to reduce size
    if (id == PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR) {
        err = pbio_motor_set_dc_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_run_settings(mtr, int_fix16_div(1200, ratio), int_fix16_div(2400, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_pid_settings(mtr, 400, 600, 5, 100, 3, 5, 2, 200);
        if (err != PBIO_SUCCESS) { return err; }
    }
    else if (id == PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR) {
        err = pbio_motor_set_dc_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_run_settings(mtr, int_fix16_div(800, ratio), int_fix16_div(1600, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_pid_settings(mtr, 500, 800, 5, 100, 3, 5, 2, 200);
        if (err != PBIO_SUCCESS) { return err; }
    }
    else if (id == PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR) {
        err = pbio_motor_set_dc_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_run_settings(mtr, int_fix16_div(1500, ratio), int_fix16_div(3000, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_pid_settings(mtr, 400, 600, 5, 100, 3, 5, 2, 200);
        if (err != PBIO_SUCCESS) { return err; }
    }
    else {
        // Defaults
        err = pbio_motor_set_dc_settings(mtr, 100, 0);
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_run_settings(mtr, int_fix16_div(1000, ratio), int_fix16_div(1000, ratio));
        if (err != PBIO_SUCCESS) { return err; }
        err = pbio_motor_set_pid_settings(mtr, 500, 800, 5, 100, 3, 5, 2, 500);
        if (err != PBIO_SUCCESS) { return err; }
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_set_dc_settings(pbio_motor_t *mtr, int32_t stall_torque_limit_pct, int32_t duty_offset_pct) {
    if (stall_torque_limit_pct < 0 || duty_offset_pct < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    mtr->max_duty_steps = PBIO_DUTY_STEPS_PER_USER_STEP * stall_torque_limit_pct;
    mtr->duty_offset = PBIO_DUTY_STEPS_PER_USER_STEP * duty_offset_pct;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_get_dc_settings(pbio_motor_t *mtr, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct) {
    *stall_torque_limit_pct = mtr->max_duty_steps/PBIO_DUTY_STEPS_PER_USER_STEP;
    *duty_offset_pct = mtr->duty_offset/PBIO_DUTY_STEPS_PER_USER_STEP;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_set_run_settings(pbio_motor_t *mtr, int32_t max_speed, int32_t acceleration) {
    mtr->control.settings.max_rate = int_fix16_mul(max_speed, mtr->counts_per_output_unit);
    mtr->control.settings.abs_acceleration = int_fix16_mul(acceleration, mtr->counts_per_output_unit);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_set_pid_settings(pbio_motor_t *mtr,
                                         int16_t pid_kp,
                                         int16_t pid_ki,
                                         int16_t pid_kd,
                                         int32_t tight_loop_time,
                                         int32_t position_tolerance,
                                         int32_t speed_tolerance,
                                         int32_t stall_speed_limit,
                                         int32_t stall_time) {
    fix16_t counts_per_output_unit = mtr->counts_per_output_unit;

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

void pbio_motor_print_settings(pbio_motor_t *mtr, char *dc_settings_string, char *enc_settings_string) {
    char *direction = mtr->direction == PBIO_MOTOR_DIR_CLOCKWISE ? "clockwise" : "counterclockwise";
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
        fix16_t counts_per_output_unit = mtr->counts_per_output_unit;
        fix16_t counts_per_unit = mtr->counts_per_unit;
        fix16_t gear_ratio = fix16_div(counts_per_output_unit, mtr->counts_per_unit);
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
            (int32_t) (mtr->max_duty_steps / PBIO_DUTY_STEPS_PER_USER_STEP),
            (int32_t) (mtr->duty_offset / PBIO_DUTY_STEPS_PER_USER_STEP),
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

bool pbio_motor_has_encoder(pbio_motor_t *mtr) {
    return mtr->has_encoders;
}

pbio_error_t pbio_motor_get_encoder_count(pbio_motor_t *mtr, int32_t *count) {
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

    if (mtr->direction == PBIO_MOTOR_DIR_COUNTERCLOCKWISE) {
        *count = -*count;
    }
    *count -= mtr->offset;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_reset_encoder_count(pbio_motor_t *mtr, int32_t reset_count) {
    int32_t count_no_offset;
    pbio_error_t err;

    // First get the counter value without any offsets, but with the appropriate polarity/sign.
    err = pbio_motor_get_encoder_count(mtr, &count_no_offset);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    count_no_offset += mtr->offset;

    // Calculate the new offset
    mtr->offset = count_no_offset - reset_count;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_get_angle(pbio_motor_t *mtr, int32_t *angle) {
    int32_t encoder_count;
    pbio_error_t err;

    err = pbio_motor_get_encoder_count(mtr, &encoder_count);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *angle = int_fix16_div(encoder_count, mtr->counts_per_output_unit);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_reset_angle(pbio_motor_t *mtr, int32_t reset_angle) {
    // Load motor settings and status
    pbio_error_t err;

    // Perform angle reset in case of tracking / holding
    if (mtr->state == PBIO_CONTROL_ANGLE_BACKGROUND && mtr->control.action == TRACK_TARGET) {
        // Get the old angle
        int32_t angle_old;
        err = pbio_motor_get_encoder_count(mtr, &angle_old);
        if (err != PBIO_SUCCESS) { return err; }
        // Get the old target
        int32_t target_old = int_fix16_div(mtr->control.trajectory.th3, mtr->counts_per_output_unit);
        // Reset the angle
        err = pbio_motor_reset_encoder_count(mtr, int_fix16_mul(reset_angle, mtr->counts_per_output_unit));
        if (err != PBIO_SUCCESS) { return err; }
        // Set the new target based on the old angle and the old target, after the angle reset
        int32_t new_target = reset_angle + target_old - angle_old;
        return pbio_motor_track_target(mtr, new_target);

    }
    // If the motor was in a passive mode (coast, brake, user duty), reset angle and leave state unchanged
    else if (mtr->state <= PBIO_CONTROL_USRDUTY){
        return pbio_motor_reset_encoder_count(mtr, int_fix16_mul(reset_angle, mtr->counts_per_output_unit));
    }
    // In all other cases, stop the ongoing maneuver by coasting and then reset the angle
    else {
        err = pbio_motor_coast(mtr);
        if (err != PBIO_SUCCESS) { return err; }
        return pbio_motor_reset_encoder_count(mtr, int_fix16_mul(reset_angle, mtr->counts_per_output_unit));
    }
}

pbio_error_t pbio_motor_get_encoder_rate(pbio_motor_t *mtr, int32_t *rate) {
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

    if (mtr->direction == PBIO_MOTOR_DIR_COUNTERCLOCKWISE) {
        *rate = -*rate;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_motor_get_angular_rate(pbio_motor_t *mtr, int32_t *angular_rate) {
    int32_t encoder_rate;
    pbio_error_t err;

    err = pbio_motor_get_encoder_rate(mtr, &encoder_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *angular_rate = int_fix16_div(encoder_rate, mtr->counts_per_output_unit);

    return PBIO_SUCCESS;
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
