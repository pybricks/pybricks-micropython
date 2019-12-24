// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <contiki.h>

#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

#define DRIVEBASE_LOG_NUM_VALUES (7 + NUM_DEFAULT_LOG_VALUES)

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

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

    err = pbio_hbridge_set_duty_cycle_sys(db->left->hbridge, sum_duty + dif_duty);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_hbridge_set_duty_cycle_sys(db->right->hbridge, sum_duty - dif_duty);
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
    err = pbio_control_set_limits(&db->control_heading.settings, db->dif_per_deg, 45, 20);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_set_pid_settings(&db->control_heading.settings, db->dif_per_deg, 100, 0, 5, 100, 2, 5, 5, 200);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Configure distance controller
    err = pbio_control_set_limits(&db->control_distance.settings, db->sum_per_mm, 100, 50);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_set_pid_settings(&db->control_distance.settings, db->sum_per_mm, 100, 0, 5, 100, 2, 5, 5, 200);
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

pbio_error_t pbio_drivebase_start(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate) {

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

    pbio_control_trajectory_t *heading_traj = &db->control_heading.trajectory;
    err = make_trajectory_time_based_forever(
        time_now,
        dif,
        dif_rate,
        pbio_math_mul_i32_fix16(turn_rate, db->dif_per_deg),
        db->control_heading.settings.max_rate,
        db->control_heading.settings.abs_acceleration,
        heading_traj);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Set distance maneuver action and stop type
    db->control_distance.after_stop = PBIO_ACTUATION_COAST;
    db->control_distance.is_done_func = pbio_control_never_done;

    pbio_control_trajectory_t *distance_traj = &db->control_distance.trajectory;
    err = make_trajectory_time_based_forever(
        time_now,
        sum,
        sum_rate,
        pbio_math_mul_i32_fix16(speed, db->sum_per_mm),
        db->control_distance.settings.max_rate,
        db->control_distance.settings.abs_acceleration,
        distance_traj);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Initialize or reset the PID control status for the given maneuver
    pbio_rate_integrator_reset(&db->control_distance.rate_integrator, 0, distance_traj->th0, distance_traj->th0);
    pbio_rate_integrator_reset(&db->control_heading.rate_integrator, 0, heading_traj->th0, heading_traj->th0);

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
