// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <contiki.h>

#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

// Number of resolution counts per degree and mm drivebase motion
#define COUNTS_PER_DEGREE (10)
#define COUNTS_PER_MM (10)

#define DRIVEBASE_LOG_NUM_VALUES (7 + NUM_DEFAULT_LOG_VALUES)

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_drivebase_t __db;

// Get the physical state of a drivebase
static pbio_error_t drivebase_get_state(pbio_drivebase_t *db,
                                        int32_t *time_now,
                                        int32_t *distance_count,
                                        int32_t *distance_rate_count,
                                        int32_t *heading_count,
                                        int32_t *heading_rate_count) {

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

    int32_t rate_left;
    err = pbio_tacho_get_angular_rate(db->left->tacho, &rate_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t rate_right;
    err = pbio_tacho_get_angular_rate(db->right->tacho, &rate_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *distance_count = pbio_math_mul_i32_fix16(angle_left + angle_right, db->drive_counts_per_sum);
    *distance_rate_count = pbio_math_mul_i32_fix16(rate_left + rate_right, db->drive_counts_per_sum);

    *heading_count = pbio_math_mul_i32_fix16(angle_left - angle_right, db->turn_counts_per_diff);
    *heading_rate_count = pbio_math_mul_i32_fix16(rate_left - rate_right, db->turn_counts_per_diff);

    return PBIO_SUCCESS;
}

// Get the physical state of a drivebase
static pbio_error_t drivebase_actuate(pbio_drivebase_t *db, int32_t distance_control, int32_t heading_control) {
    pbio_error_t err;

    int32_t dif = pbio_math_mul_i32_fix16(heading_control, db->turn_counts_per_diff);
    int32_t sum = pbio_math_mul_i32_fix16(distance_control, db->drive_counts_per_sum);
    
    err = pbio_hbridge_set_duty_cycle_sys(db->left->hbridge, sum + dif);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_hbridge_set_duty_cycle_sys(db->right->hbridge, sum - dif);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_SUCCESS;
}

// Log motor data for a motor that is being actively controlled
static pbio_error_t drivebase_log_update(pbio_drivebase_t *db, 
                                         int32_t time_now,
                                         int32_t distance_count,
                                         int32_t distance_rate_count,
                                         int32_t distance_control,
                                         int32_t heading_count,
                                         int32_t heading_rate_count,
                                         int32_t heading_control) {

    int32_t buf[DRIVEBASE_LOG_NUM_VALUES];
    buf[0] = time_now;
    buf[1] = distance_count;
    buf[2] = distance_rate_count;
    buf[3] = distance_control;
    buf[4] = heading_count;
    buf[5] = heading_rate_count;
    buf[6] = heading_control;
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

    // Turn counts for every degree difference between the servo motors
    db->turn_counts_per_diff = fix16_div(
        fix16_mul(db->wheel_diameter, fix16_from_int(COUNTS_PER_DEGREE)),
        fix16_mul(db->axle_track, fix16_from_int(2))
    );

    // Forward drive counts for every summed degree of the servo motors
    db->drive_counts_per_sum = fix16_div(
        fix16_mul(fix16_mul(db->wheel_diameter, fix16_pi), fix16_from_int(COUNTS_PER_MM)),
        fix16_from_int(720)
    );

    // Claim servos
    db->left->state = PBIO_SERVO_STATE_CLAIMED;
    db->right->state = PBIO_SERVO_STATE_CLAIMED;

    // Initialize log
    db->log.num_values = DRIVEBASE_LOG_NUM_VALUES;

    // Configure heading controller
    err = pbio_control_set_limits(&db->control_heading.settings, fix16_from_int(COUNTS_PER_DEGREE), 45, 20);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_set_pid_settings(&db->control_heading.settings, fix16_from_int(COUNTS_PER_DEGREE), 100, 1, 1, 100, 2, 5, 5, 200);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Configure distance controller
    err = pbio_control_set_limits(&db->control_distance.settings, fix16_from_int(COUNTS_PER_MM), 100, 50);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_set_pid_settings(&db->control_distance.settings, fix16_from_int(COUNTS_PER_MM), 100, 1, 1, 100, 2, 5, 5, 200);
    if (err != PBIO_SUCCESS) {
        return err;
    }

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
            err = pbio_hbridge_coast(db->left->hbridge);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_hbridge_coast(db->right->hbridge);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            db->state = PBIO_DRIVEBASE_STATE_PASSIVE;
            return PBIO_SUCCESS;
        case PBIO_ACTUATION_BRAKE:
            // Stop by braking
            err = pbio_hbridge_brake(db->left->hbridge);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_hbridge_brake(db->right->hbridge);
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
    int32_t time_now, distance_count, distance_rate_count, heading_count, heading_rate_count;
    pbio_error_t err = drivebase_get_state(db, &time_now, &distance_count, &distance_rate_count, &heading_count, &heading_rate_count);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t distance_control, heading_control;

    if (db->state == PBIO_DRIVEBASE_STATE_PASSIVE) {
        // When passive, zero control
        distance_control = 0;
        heading_control = 0;
    }
    else {
        // Otherwise, calculate control signal
        pbio_actuation_t __todo; // FIXME: add other control types
        err = control_update_time_target(&db->control_distance, time_now, distance_count, distance_rate_count, &__todo, &distance_control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        err = control_update_time_target(&db->control_heading, time_now, heading_count, heading_rate_count, &__todo, &heading_control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Actuate
    err = drivebase_actuate(db, distance_control, heading_control);

    // Log state and control
    return drivebase_log_update(db, time_now, distance_count, distance_rate_count, distance_control, heading_count, heading_rate_count, heading_control);
}

pbio_error_t pbio_drivebase_start(pbio_drivebase_t *db, int32_t speed, int32_t rate) {

    pbio_error_t err;

    // Get the physical initial state
    int32_t time_now, distance_count, distance_rate_count, heading_count, heading_rate_count;
    err = drivebase_get_state(db, &time_now, &distance_count, &distance_rate_count, &heading_count, &heading_rate_count);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set heading maneuver action and stop type
    db->control_heading.action = RUN;
    db->control_heading.after_stop = PBIO_ACTUATION_COAST;
    err = make_trajectory_time_based_forever(
        time_now,
        heading_count,
        heading_rate_count,
        pbio_math_mul_i32_fix16(rate, fix16_from_int(COUNTS_PER_DEGREE)),
        db->control_heading.settings.max_rate,
        db->control_heading.settings.abs_acceleration,
        &db->control_heading.trajectory);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set distance maneuver action and stop type
    db->control_distance.action = RUN;
    db->control_distance.after_stop = PBIO_ACTUATION_COAST;
    err = make_trajectory_time_based_forever(
        time_now,
        distance_count,
        distance_rate_count,
        pbio_math_mul_i32_fix16(speed, fix16_from_int(COUNTS_PER_MM)),
        db->control_distance.settings.max_rate,
        db->control_distance.settings.abs_acceleration,
        &db->control_distance.trajectory);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(&db->control_heading);
    control_init_time_target(&db->control_distance);

    db->state = PBIO_DRIVEBASE_STATE_TIME_BACKGROUND;

    // Run one control update synchronously with user command.
    err = pbio_drivebase_update(db);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
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
