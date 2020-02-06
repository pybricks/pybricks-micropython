// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <contiki.h>

#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

#define DRIVEBASE_LOG_NUM_VALUES (15 + NUM_DEFAULT_LOG_VALUES)

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_control_settings_t settings_drivebase_heading_default = {
    .max_rate = 360,
    .abs_acceleration = 720,
    .rate_tolerance = 8,
    .count_tolerance = 8,
    .stall_rate_limit = 8,
    .stall_time = 200,
    .pid_kp = 200,
    .pid_ki = 0,
    .pid_kd = 12,
    .max_control = 10000,
};

static pbio_control_settings_t settings_drivebase_distance_default = {
    .max_rate = 800,
    .abs_acceleration = 1600,
    .rate_tolerance = 8,
    .count_tolerance = 8,
    .stall_rate_limit = 8,
    .stall_time = 200,
    .pid_kp = 200,
    .pid_ki = 0,
    .pid_kd = 10,
    .max_control = 10000,
};

static pbio_drivebase_t __db;

// Get the physical state of a drivebase
static pbio_error_t drivebase_get_state(pbio_drivebase_t *db,
                                        int32_t *time_now,
                                        int32_t *sum,
                                        int32_t *sum_rate,
                                        int32_t *dif,
                                        int32_t *dif_rate) {

    pbio_error_t err;

    // Read current state of this motor: current time, speed, and position
    *time_now = clock_usecs();

    int32_t angle_left;
    err = pbio_tacho_get_angle(db->left->tacho, &angle_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t angle_right;
    err = pbio_tacho_get_angle(db->right->tacho, &angle_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t angular_rate_left;
    err = pbio_tacho_get_angular_rate(db->left->tacho, &angular_rate_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t angular_rate_right;
    err = pbio_tacho_get_angular_rate(db->right->tacho, &angular_rate_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *sum = angle_left + angle_right;
    *sum_rate = angular_rate_left + angular_rate_right;
    *dif = angle_left - angle_right;
    *dif_rate = angular_rate_left - angular_rate_right;

    return PBIO_SUCCESS;
}

// Get the physical state of a drivebase
static pbio_error_t drivebase_actuate(pbio_drivebase_t *db, int32_t sum_control, int32_t dif_control) {
    pbio_error_t err;

    // FIXME: scale only once, embed max control in prescaler
    int32_t sum_duty = 4*(sum_control*100)/pbio_math_mul_i32_fix16(100, db->sum_per_mm);
    int32_t dif_duty = 4*(dif_control*100)/pbio_math_mul_i32_fix16(100, db->dif_per_deg);

    err = pbio_dcmotor_set_duty_cycle_sys(db->left->dcmotor, sum_duty + dif_duty);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_dcmotor_set_duty_cycle_sys(db->right->dcmotor, sum_duty - dif_duty);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_SUCCESS;
}

// Log motor data for a motor that is being actively controlled
static pbio_error_t drivebase_log_update(pbio_drivebase_t *db, 
                                         int32_t time_now,
                                         int32_t sum,
                                         int32_t sum_rate,
                                         int32_t sum_control,
                                         int32_t dif,
                                         int32_t dif_rate,
                                         int32_t dif_control) {

    int32_t buf[DRIVEBASE_LOG_NUM_VALUES];
    buf[0] = time_now;
    buf[1] = sum;
    buf[2] = sum_rate;
    buf[3] = sum_control;
    buf[4] = dif;
    buf[5] = dif_rate;
    buf[6] = dif_control;

    int32_t sum_ref, sum_ref_ext, sum_rate_ref, sum_rate_err, sum_rate_err_integral, sum_acceleration_ref;
    pbio_trajectory_get_reference(&db->control_distance.trajectory, time_now, &sum_ref, &sum_ref_ext, &sum_rate_ref, &sum_acceleration_ref);
    pbio_rate_integrator_get_errors(&db->control_distance.rate_integrator, time_now, sum_rate_ref, sum, sum_ref, &sum_rate_err, &sum_rate_err_integral);
    buf[7] = sum_ref;
    buf[8] = sum_rate_err_integral;
    buf[9] = sum_rate_ref;
    buf[10] = sum_rate_err_integral;

    int32_t dif_ref, dif_ref_ext, dif_rate_ref, dif_rate_err, dif_rate_err_integral, dif_acceleration_ref;
    pbio_trajectory_get_reference(&db->control_heading.trajectory, time_now, &dif_ref, &dif_ref_ext, &dif_rate_ref, &dif_acceleration_ref);
    pbio_rate_integrator_get_errors(&db->control_heading.rate_integrator, time_now, dif_rate_ref, dif, dif_ref, &dif_rate_err, &dif_rate_err_integral);
    buf[11] = dif_ref;
    buf[12] = dif_rate_err_integral;
    buf[13] = dif_rate_ref;
    buf[14] = dif_rate_err_integral;

    return pbio_logger_update(&db->log, buf);
}

static pbio_error_t pbio_drivebase_setup(pbio_drivebase_t *db,
                                         pbio_servo_t *left,
                                         pbio_servo_t *right,
                                         fix16_t wheel_diameter,
                                         fix16_t axle_track) {
    pbio_error_t err;

    // Reset both motors to a passive state
    err = pbio_servo_stop(left, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_stop(right, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Individual servos
    db->left = left;
    db->right = right;

    // Drivebase geometry
    if (wheel_diameter <= 0 || axle_track <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    db->wheel_diameter = wheel_diameter;
    db->axle_track = axle_track;

    // Difference between the motors for every 1 degree drivebase rotation
    db->dif_per_deg = fix16_div(
        fix16_mul(db->axle_track, fix16_from_int(2)),
        db->wheel_diameter
    );

    // Sum of motors for every mm forward
    db->sum_per_mm = fix16_div(
        fix16_mul(fix16_from_int(180), FOUR_DIV_PI),
        db->wheel_diameter
    );

    // Claim servos
    db->left->state = PBIO_SERVO_STATE_CLAIMED;
    db->right->state = PBIO_SERVO_STATE_CLAIMED;

    // Initialize log
    db->log.num_values = DRIVEBASE_LOG_NUM_VALUES;

    // Configure heading controller
    db->control_heading.settings = settings_drivebase_heading_default;
    db->control_heading.settings.counts_per_unit = db->dif_per_deg;

    // Configure distance controller
    db->control_distance.settings = settings_drivebase_distance_default;
    db->control_distance.settings.counts_per_unit = db->sum_per_mm;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_get(pbio_drivebase_t **_db, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track) {

    // Get pointer to device
    pbio_drivebase_t *db = &__db;

    // Configure drivebase and set properties
    pbio_error_t err = pbio_drivebase_setup(db, left, right, wheel_diameter, axle_track);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // On success, return pointer to device
    *_db = db;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_actuation_t after_stop) {

    pbio_error_t err;

    switch (after_stop) {
        case PBIO_ACTUATION_COAST:
            // Stop by coasting
            err = pbio_dcmotor_coast(db->left->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_dcmotor_coast(db->right->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            db->state = PBIO_DRIVEBASE_STATE_PASSIVE;
            return PBIO_SUCCESS;
        case PBIO_ACTUATION_BRAKE:
            // Stop by braking
            err = pbio_dcmotor_brake(db->left->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_dcmotor_brake(db->right->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            db->state = PBIO_DRIVEBASE_STATE_PASSIVE;
            return PBIO_SUCCESS;
        default:
            // HOLD is not implemented
            return PBIO_ERROR_INVALID_ARG;
    }
}

static pbio_error_t pbio_drivebase_update(pbio_drivebase_t *db) {
    // Get the physical state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    pbio_error_t err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t sum_control, dif_control;

    if (db->state == PBIO_DRIVEBASE_STATE_PASSIVE) {
        // When passive, zero control
        sum_control = 0;
        dif_control = 0;
    }
    else {
        // Otherwise, calculate control signal
        pbio_actuation_t __todo; // FIXME: add other control types
        control_update_time_target(&db->control_distance, time_now, sum, sum_rate, &__todo, &sum_control);
        control_update_time_target(&db->control_heading, time_now, dif, dif_rate, &__todo, &dif_control);
    }

    // Actuate
    err = drivebase_actuate(db, sum_control, dif_control);

    // Log state and control
    return drivebase_log_update(db, time_now, sum, sum_rate, sum_control, dif, dif_rate, dif_control);
}

static pbio_error_t pbio_drivebase_signal_run(pbio_control_t *ctl, pbio_drivebase_state_t state, int32_t target_rate, int32_t time_now, int32_t count_now, int32_t rate_now) {

    pbio_error_t err;

    // If we are continuing a timed maneuver, we can try to patch the new command onto the existing one for better continuity
    if (state == PBIO_DRIVEBASE_STATE_TIMED) {

        // Make the new trajectory and try to patch
        err = pbio_trajectory_make_time_based_patched(
            &ctl->trajectory,
            true,
            time_now,
            time_now,
            target_rate,
            ctl->settings.max_rate,
            ctl->settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    else {
        // If angle based or no maneuver was ongoing, make a basic new trajectory
        // Based on the current time and current state
        err = pbio_trajectory_make_time_based(
            &ctl->trajectory,
            true,
            time_now,
            time_now,
            count_now,
            0,
            rate_now,
            target_rate,
            ctl->settings.max_rate,
            ctl->settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // New maneuver, so reset the rate integrator
        pbio_rate_integrator_reset(&ctl->rate_integrator, time_now, count_now, count_now);
    }

    // Set new maneuver action and stop type, and state
    ctl->after_stop = PBIO_ACTUATION_COAST;
    ctl->is_done_func = pbio_control_never_done;
    
    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_straight(pbio_drivebase_t *db, int32_t distance) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbio_drivebase_turn(pbio_drivebase_t *db, int32_t angle) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbio_drivebase_drive(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate) {

    pbio_error_t err;

    // Get the physical initial state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set heading maneuver action and stop type
    db->control_heading.after_stop = PBIO_ACTUATION_COAST;
    db->control_heading.is_done_func = pbio_control_never_done;

    // Initialize both controllers
    int32_t target_turn_rate = pbio_math_mul_i32_fix16(turn_rate, db->dif_per_deg);
    err = pbio_drivebase_signal_run(&db->control_heading, db->state, target_turn_rate, time_now, dif, dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    int32_t target_sum_rate = pbio_math_mul_i32_fix16(speed, db->sum_per_mm);
    err = pbio_drivebase_signal_run(&db->control_distance, db->state, target_sum_rate, time_now, sum, sum_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    db->state = PBIO_DRIVEBASE_STATE_TIMED;

    // Run one control update synchronously with user command.
    err = pbio_drivebase_update(db);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_get_state(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate) {
    *distance = 0;
    *drive_speed = 0;
    *angle = 0;
    *turn_rate = 0;
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbio_drivebase_reset(pbio_drivebase_t *db) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t turn_rate, int32_t turn_acceleration, pbio_actuation_t stop_type) {
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

pbio_error_t pbio_drivebase_get_drive_settings(pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *turn_rate, int32_t *turn_acceleration, pbio_actuation_t *stop_type) {
    *drive_speed = 0;
    *drive_acceleration = 0;
    *turn_rate = 0;
    *turn_acceleration = 0;
    *stop_type = PBIO_ACTUATION_COAST;
    return PBIO_ERROR_NOT_IMPLEMENTED;
}

// TODO: Convert to Contiki process

// Service all drivebase motors by calling this function at approximately constant intervals.
void _pbio_drivebase_poll(void) {
    pbio_drivebase_t *db = &__db;

    if (db->left && db->left->connected && db->right && db->right->connected) {
        pbio_drivebase_update(db);
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
