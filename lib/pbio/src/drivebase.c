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
static pbio_error_t pbio_drivebase_actuate(pbio_drivebase_t *db, pbio_actuation_t sum_actuation, int32_t sum_control, pbio_actuation_t dif_actuation, int32_t dif_control) {
    pbio_error_t err;

    // If either signal coasts, both must coast
    if (sum_actuation == PBIO_ACTUATION_COAST || dif_actuation == PBIO_ACTUATION_COAST) {
        // Coast motors
        err = pbio_dcmotor_coast(db->left->dcmotor);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        err = pbio_dcmotor_coast(db->left->dcmotor);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return PBIO_SUCCESS;
    }

    // Hold is not yet implemented
    if (sum_actuation == PBIO_ACTUATION_HOLD || dif_actuation == PBIO_ACTUATION_HOLD) {
       return PBIO_ERROR_NOT_IMPLEMENTED;
    }

    // Brake is the same as duty, so just actuate
    err = pbio_dcmotor_set_duty_cycle_sys(db->left->dcmotor, sum_control + dif_control);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_dcmotor_set_duty_cycle_sys(db->right->dcmotor, sum_control - dif_control);
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

    // Initialize log
    db->log.num_values = DRIVEBASE_LOG_NUM_VALUES;

    // Configure heading controller
    db->control_heading.settings = settings_drivebase_heading_default;

    // Difference between the motors for every 1 degree drivebase rotation
    db->control_heading.settings.counts_per_unit = fix16_div(
        fix16_mul(db->axle_track, fix16_from_int(2)),
        db->wheel_diameter
    );

    // Configure distance controller
    db->control_distance.settings = settings_drivebase_distance_default;

    // Sum of motors for every mm forward
    db->control_distance.settings.counts_per_unit = fix16_div(
        fix16_mul(fix16_from_int(180), FOUR_DIV_PI),
        db->wheel_diameter
    );

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

    int32_t sum_control;
    int32_t dif_control;

    if (after_stop == PBIO_ACTUATION_HOLD) {
        // When holding, the control payload is the count to hold
        int32_t unused;
        err = drivebase_get_state(db, &unused, &sum_control, &unused, &dif_control, &unused);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    else {
        // Otherwise the payload is zero and control stops
        pbio_control_stop(&db->control_distance);
        pbio_control_stop(&db->control_heading);
        sum_control = 0;
        dif_control = 0;
    }

    return pbio_drivebase_actuate(db, after_stop, sum_control, after_stop, dif_control);
}

static pbio_error_t pbio_drivebase_update(pbio_drivebase_t *db) {
    // Get the physical state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    pbio_error_t err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If passive, log and exit
    if (db->control_heading.type == PBIO_CONTROL_NONE || db->control_distance.type == PBIO_CONTROL_NONE) {
        return drivebase_log_update(db, time_now, sum, sum_rate, 0, dif, dif_rate, 0);
    }

    // Get control signals
    int32_t sum_control, dif_control;
    pbio_actuation_t sum_actuation, dif_actuation;
    control_update(&db->control_distance, time_now, sum, sum_rate, &sum_actuation, &sum_control);
    control_update(&db->control_heading, time_now, dif, dif_rate, &dif_actuation, &dif_control);

    // Actuate
    err = pbio_drivebase_actuate(db, sum_actuation, sum_control, dif_actuation, dif_control);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return drivebase_log_update(db, time_now, sum, sum_rate, sum_control, dif, dif_rate, dif_control);
}

pbio_error_t pbio_drivebase_straight(pbio_drivebase_t *db, int32_t distance) {

    pbio_error_t err;

    // Get the physical initial state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Targets in units of count
    int32_t relative_sum_target = pbio_control_user_to_counts(&db->control_distance.settings, distance);
    int32_t target_sum_rate = pbio_control_user_to_counts(&db->control_distance.settings, db->control_distance.settings.max_rate);
    int32_t target_dif_rate = pbio_control_user_to_counts(&db->control_heading.settings, db->control_heading.settings.max_rate);

    // Initialize both controllers
    err = pbio_control_start_relative_angle_control(&db->control_distance, time_now, sum, relative_sum_target, sum_rate, target_sum_rate, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_start_relative_angle_control(&db->control_heading, time_now, dif, 0, dif_rate, target_dif_rate, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_turn(pbio_drivebase_t *db, int32_t angle) {

    pbio_error_t err;

    // Get the physical initial state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Targets in units of count
    int32_t relative_dif_target = pbio_control_user_to_counts(&db->control_heading.settings, angle);
    int32_t target_sum_rate = pbio_control_user_to_counts(&db->control_distance.settings, db->control_distance.settings.max_rate);
    int32_t target_dif_rate = pbio_control_user_to_counts(&db->control_heading.settings, db->control_heading.settings.max_rate);

    // Initialize both controllers
    err = pbio_control_start_relative_angle_control(&db->control_distance, time_now, sum, 0, sum_rate, target_sum_rate, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_control_start_relative_angle_control(&db->control_heading, time_now, dif, relative_dif_target, dif_rate, target_dif_rate, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}
pbio_error_t pbio_drivebase_drive(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate) {

    pbio_error_t err;

    // Get the physical initial state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Initialize both controllers
    int32_t target_turn_rate = pbio_control_user_to_counts(&db->control_heading.settings, turn_rate);
    err = pbio_control_start_timed_control(&db->control_heading, time_now, DURATION_FOREVER, dif, dif_rate, target_turn_rate, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    int32_t target_sum_rate = pbio_control_user_to_counts(&db->control_distance.settings, speed);
    err = pbio_control_start_timed_control(&db->control_distance, time_now, DURATION_FOREVER, sum, sum_rate, target_sum_rate, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_get_state(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate) {
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    pbio_error_t err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *distance =    pbio_control_counts_to_user(&db->control_distance.settings, sum - db->sum_offset);
    *drive_speed = pbio_control_counts_to_user(&db->control_distance.settings, sum_rate);
    *angle =       pbio_control_counts_to_user(&db->control_heading.settings,  dif - db->dif_offset);
    *turn_rate =   pbio_control_counts_to_user(&db->control_heading.settings,  dif_rate);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_reset(pbio_drivebase_t *db) {
    int32_t time_now, sum_rate, dif_rate;
    return drivebase_get_state(db, &time_now, &db->sum_offset, &sum_rate, &db->dif_offset, &dif_rate);
}

pbio_error_t pbio_drivebase_get_drive_settings(pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *turn_rate, int32_t *turn_acceleration) {

    pbio_control_settings_t *sd = &db->control_distance.settings;
    pbio_control_settings_t *sh = &db->control_heading.settings;

    *drive_speed = pbio_control_counts_to_user(sd, sd->max_rate);
    *drive_acceleration = pbio_control_counts_to_user(sd, sd->abs_acceleration);
    *turn_rate = pbio_control_counts_to_user(sh, sh->max_rate);
    *turn_acceleration = pbio_control_counts_to_user(sh, sh->abs_acceleration);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t turn_rate, int32_t turn_acceleration) {

    pbio_control_settings_t *sd = &db->control_distance.settings;
    pbio_control_settings_t *sh = &db->control_heading.settings;

    sd->max_rate = pbio_control_user_to_counts(sd, drive_speed);
    sd->abs_acceleration = pbio_control_user_to_counts(sd, drive_acceleration);
    sh->max_rate = pbio_control_user_to_counts(sh, turn_rate);
    sh->abs_acceleration = pbio_control_user_to_counts(sh, turn_acceleration);

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
