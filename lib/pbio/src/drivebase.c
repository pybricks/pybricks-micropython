// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <contiki.h>

#include <pbdrv/battery.h>

#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_error_t drivebase_adopt_settings(pbio_control_settings_t *s_distance, pbio_control_settings_t *s_heading, pbio_control_settings_t *s_left, pbio_control_settings_t *s_right) {

    // All rate/count acceleration limits add up, because distance state is two motors counts added
    s_distance->max_rate = s_left->max_rate + s_right->max_rate;
    s_distance->rate_tolerance = s_left->rate_tolerance + s_right->rate_tolerance;
    s_distance->count_tolerance = s_left->count_tolerance + s_right->count_tolerance;
    s_distance->stall_rate_limit = s_left->stall_rate_limit + s_right->stall_rate_limit;
    s_distance->integral_range = s_left->integral_range + s_right->integral_range;
    s_distance->integral_rate = s_left->integral_rate + s_right->integral_rate;

    // As acceleration, we take double the single motor amount, because drivebases are
    // usually expected to respond quickly to speed setpoint changes
    s_distance->abs_acceleration = (s_left->abs_acceleration + s_right->abs_acceleration) * 2;

    // Use the average PID of both motors
    s_distance->pid_kp = (s_left->pid_kp + s_right->pid_kp) / 2;
    s_distance->pid_ki = (s_left->pid_ki + s_right->pid_ki) / 2;
    s_distance->pid_kd = (s_left->pid_kd + s_right->pid_kd) / 2;

    // Maxima are bound by the least capable motor
    s_distance->max_torque = min(s_left->max_torque, s_right->max_torque);
    s_distance->stall_time = min(s_left->stall_time, s_right->stall_time);

    // Copy rate estimator usage, required to be the same on both motors
    if (s_left->use_estimated_rate != s_right->use_estimated_rate || s_left->use_estimated_count != s_right->use_estimated_count) {
        return PBIO_ERROR_INVALID_ARG;
    }
    s_distance->use_estimated_rate = s_left->use_estimated_rate;

    // Use estimated count if we use estimated rate
    s_distance->use_estimated_count = s_distance->use_estimated_rate;

    // By default, heading control is the same as distance control
    *s_heading = *s_distance;

    // Allow just slightly more torque for heading. While not technically
    // necessary under nominal circumstances, it gives the expected perceived
    // result of one wheel nearly stopping when you block the other.
    s_heading->max_torque *= 2;

    return PBIO_SUCCESS;
}

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

    int32_t count_left;
    err = pbio_tacho_get_count(db->left->tacho, &count_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t count_right;
    err = pbio_tacho_get_count(db->right->tacho, &count_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t rate_left;
    err = pbio_tacho_get_rate(db->left->tacho, &rate_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t rate_right;
    err = pbio_tacho_get_rate(db->right->tacho, &rate_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *sum = count_left + count_right;
    *sum_rate = rate_left + rate_right;
    *dif = count_left - count_right;
    *dif_rate = rate_left - rate_right;

    return PBIO_SUCCESS;
}

// Get the estimated state of a drivebase
static void drivebase_get_estimated_state(pbio_drivebase_t *db,
    int32_t *sum,
    int32_t *sum_rate,
    int32_t *dif,
    int32_t *dif_rate) {

    int32_t count_left, rate_left;
    pbio_observer_get_estimated_state(&db->left->observer, &count_left, &rate_left);

    int32_t count_right, rate_right;
    pbio_observer_get_estimated_state(&db->right->observer, &count_right, &rate_right);

    *sum = count_left + count_right;
    *sum_rate = rate_left + rate_right;
    *dif = count_left - count_right;
    *dif_rate = rate_left - rate_right;
}

// Actuate a drivebase
static pbio_error_t pbio_drivebase_actuate(pbio_drivebase_t *db, pbio_actuation_t actuation, int32_t sum_control, int32_t dif_control) {
    pbio_error_t err;

    switch (actuation) {
        case PBIO_ACTUATION_COAST:
            err = pbio_dcmotor_coast(db->left->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_dcmotor_coast(db->right->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            // Release claim on servos
            pbio_drivebase_claim_servos(db, false);
            break;
        case PBIO_ACTUATION_BRAKE:
            err = pbio_dcmotor_brake(db->left->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_dcmotor_brake(db->right->dcmotor);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            // Release claim on servos
            pbio_drivebase_claim_servos(db, false);
            break;
        case PBIO_ACTUATION_HOLD:
            err = pbio_drivebase_straight(db, 0, db->control_distance.settings.max_rate, db->control_distance.settings.max_rate);
            break;
        case PBIO_ACTUATION_DUTY:
            err = pbio_dcmotor_set_duty_cycle_sys(db->left->dcmotor, sum_control + dif_control);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            err = pbio_dcmotor_set_duty_cycle_sys(db->right->dcmotor, sum_control - dif_control);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            break;
        default:
            err = PBIO_ERROR_INVALID_ARG;
            break;
    }
    return err;
}

pbio_error_t pbio_drivebase_setup(pbio_drivebase_t *db, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track) {
    pbio_error_t err;

    // Stop any existing drivebase motion
    err = pbio_drivebase_stop_force(db);
    if (!(err == PBIO_SUCCESS || err == PBIO_ERROR_NO_DEV)) {
        return err;
    }

    // Reset both motors to a passive state
    err = pbio_servo_stop(left, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_stop(right, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Drivebase geometry
    if (wheel_diameter <= 0 || axle_track <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Assert that both motors have the same gearing
    if (left->control.settings.counts_per_unit != right->control.settings.counts_per_unit) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Individual servos
    db->left = left;
    db->right = right;
    pbio_drivebase_claim_servos(db, false);

    // Adopt settings as the average or sum of both servos, except scaling
    err = drivebase_adopt_settings(&db->control_distance.settings, &db->control_heading.settings, &db->left->control.settings, &db->right->control.settings);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Count difference between the motors for every 1 degree drivebase rotation
    db->control_heading.settings.counts_per_unit =
        fix16_mul(
            left->control.settings.counts_per_unit,
            fix16_div(
                fix16_mul(
                    axle_track,
                    fix16_from_int(2)
                    ),
                wheel_diameter
                )
            );

    // Sum of motor counts for every 1 mm forward
    db->control_distance.settings.counts_per_unit =
        fix16_mul(
            left->control.settings.counts_per_unit,
            fix16_div(
                fix16_mul(
                    fix16_from_int(180),
                    FOUR_DIV_PI
                    ),
                wheel_diameter
                )
            );

    return PBIO_SUCCESS;
}

// Claim servos so that they cannot be used independently
void pbio_drivebase_claim_servos(pbio_drivebase_t *db, bool claim) {
    // Stop control
    pbio_control_stop(&db->left->control);
    pbio_control_stop(&db->right->control);
    // Set claim status
    db->left->claimed = claim;
    db->right->claimed = claim;
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
    } else {
        // Otherwise the payload is zero and control stops
        pbio_control_stop(&db->control_distance);
        pbio_control_stop(&db->control_heading);
        sum_control = 0;
        dif_control = 0;
    }

    return pbio_drivebase_actuate(db, after_stop, sum_control, dif_control);
}

pbio_error_t pbio_drivebase_stop_force(pbio_drivebase_t *db) {

    // Stop control so polling will stop
    pbio_control_stop(&db->control_distance);
    pbio_control_stop(&db->control_heading);

    pbio_error_t err;

    if (!db->left || !db->right) {
        return PBIO_ERROR_NO_DEV;
    }

    // Try to stop left servo
    err = pbio_servo_stop_force(db->left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Try to stop right servo
    return pbio_servo_stop_force(db->right);
}

pbio_error_t pbio_drivebase_update(pbio_drivebase_t *db) {

    // If passive, log and exit
    if (db->control_heading.type == PBIO_CONTROL_NONE || db->control_distance.type == PBIO_CONTROL_NONE) {
        return PBIO_SUCCESS;
    }

    // Get the physical state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    pbio_error_t err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    int32_t sum_est, sum_rate_est, dif_est, dif_rate_est;
    drivebase_get_estimated_state(db, &sum_est, &sum_rate_est, &dif_est, &dif_rate_est);

    // Get torque signals
    int32_t sum_torque, dif_torque;
    int32_t sum_rate_ref, dif_rate_ref;
    int32_t sum_acceleration_ref, dif_acceleration_ref;
    pbio_actuation_t sum_actuation, dif_actuation;
    pbio_control_update(&db->control_distance, time_now, sum, sum_rate, sum_est, sum_rate_est, &sum_actuation, &sum_torque, &sum_rate_ref, &sum_acceleration_ref);
    pbio_control_update(&db->control_heading, time_now, dif, dif_rate, dif_est, dif_rate_est, &dif_actuation, &dif_torque, &dif_rate_ref, &dif_acceleration_ref);

    // Separate actuation types are not possible for now
    if (sum_actuation != dif_actuation) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get the battery voltage
    uint16_t voltage;
    err = pbdrv_battery_get_voltage_now(&voltage);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    int32_t battery_voltage = voltage;

    // TODO: Use generic actuator with torque type.

    // Left carries sum/2 + dif/2
    int32_t torque_left = sum_torque / 2 + dif_torque / 2;
    torque_left += pbio_observer_get_feedforward_torque(&db->left->observer, sum_rate_ref / 2 + dif_rate_ref / 2, sum_acceleration_ref / 2 + dif_acceleration_ref / 2);
    int32_t duty_left = pbio_observer_torque_to_duty(&db->left->observer, torque_left, battery_voltage);

    // Right carries sum/2 - dif/2
    int32_t torque_right = sum_torque / 2 - dif_torque / 2;
    torque_right += pbio_observer_get_feedforward_torque(&db->right->observer, sum_rate_ref / 2 - dif_rate_ref / 2, sum_acceleration_ref / 2 - dif_acceleration_ref / 2);
    int32_t duty_right = pbio_observer_torque_to_duty(&db->right->observer, torque_right, battery_voltage);

    // Apply the duty cycle values
    err = pbio_dcmotor_set_duty_cycle_sys(db->left->dcmotor, duty_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return pbio_dcmotor_set_duty_cycle_sys(db->right->dcmotor, duty_right);
}

pbio_error_t pbio_drivebase_straight(pbio_drivebase_t *db, int32_t distance, int32_t drive_speed, int32_t drive_acceleration) {

    pbio_error_t err;

    // Claim both servos for use by drivebase
    pbio_drivebase_claim_servos(db, true);

    // Get the physical initial state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Sum controller performs a maneuver to drive a distance
    int32_t relative_sum_target = pbio_control_user_to_counts(&db->control_distance.settings, distance);
    int32_t target_sum_rate = pbio_control_user_to_counts(&db->control_distance.settings, drive_speed);
    int32_t sum_acceleration = pbio_control_user_to_counts(&db->control_distance.settings, drive_acceleration);

    err = pbio_control_start_relative_angle_control(&db->control_distance, time_now, sum, relative_sum_target, sum_rate, target_sum_rate, sum_acceleration, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Dif controller just holds still
    int32_t relative_dif_target = 0;
    int32_t target_dif_rate = db->control_heading.settings.max_rate;
    int32_t dif_acceleration = db->control_heading.settings.abs_acceleration;

    err = pbio_control_start_relative_angle_control(&db->control_heading, time_now, dif, relative_dif_target, dif_rate, target_dif_rate, dif_acceleration, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_turn(pbio_drivebase_t *db, int32_t angle, int32_t turn_rate, int32_t turn_acceleration) {

    pbio_error_t err;

    // Claim both servos for use by drivebase
    pbio_drivebase_claim_servos(db, true);

    // Get the physical initial state
    int32_t time_now, sum, sum_rate, dif, dif_rate;
    err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }


    // Sum controller just holds still
    int32_t relative_sum_target = 0;
    int32_t target_sum_rate = db->control_distance.settings.max_rate;
    int32_t sum_acceleration = db->control_distance.settings.abs_acceleration;

    err = pbio_control_start_relative_angle_control(&db->control_distance, time_now, sum, relative_sum_target, sum_rate, target_sum_rate, sum_acceleration, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Dif controller performs a maneuver to make a turn
    int32_t relative_dif_target = pbio_control_user_to_counts(&db->control_heading.settings, angle);
    int32_t target_dif_rate = pbio_control_user_to_counts(&db->control_heading.settings, turn_rate);
    int32_t dif_acceleration = pbio_control_user_to_counts(&db->control_heading.settings, turn_acceleration);

    err = pbio_control_start_relative_angle_control(&db->control_heading, time_now, dif, relative_dif_target, dif_rate, target_dif_rate, dif_acceleration, PBIO_ACTUATION_HOLD);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}
pbio_error_t pbio_drivebase_drive(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate) {

    pbio_error_t err;

    // Claim both servos for use by drivebase
    pbio_drivebase_claim_servos(db, true);

    // Get the physical initial state
    int32_t time_now, sum, sum_rate, dif, dif_rate;

    // FIXME: Make state getter function a control property. That way, it can
    // decide whether reading the state is needed, instead of checking control
    // status here. We do it here for now anyway to reduce I/O if the initial
    // state value is not actually used, like when control is already active.
    if (db->control_heading.type == PBIO_CONTROL_NONE && db->control_distance.type == PBIO_CONTROL_NONE) {
        // Get the current physical state.
        err = drivebase_get_state(db, &time_now, &sum, &sum_rate, &dif, &dif_rate);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        time_now = clock_usecs();
    }

    // Initialize both controllers
    int32_t target_sum_rate = pbio_control_user_to_counts(&db->control_distance.settings, speed);
    err = pbio_control_start_timed_control(&db->control_distance, time_now, DURATION_FOREVER, sum, sum_rate, target_sum_rate, db->control_distance.settings.abs_acceleration, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    int32_t target_turn_rate = pbio_control_user_to_counts(&db->control_heading.settings, turn_rate);
    err = pbio_control_start_timed_control(&db->control_heading, time_now, DURATION_FOREVER, dif, dif_rate, target_turn_rate, db->control_heading.settings.abs_acceleration, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
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
    *distance = pbio_control_counts_to_user(&db->control_distance.settings, sum - db->sum_offset);
    *drive_speed = pbio_control_counts_to_user(&db->control_distance.settings, sum_rate);
    *angle = pbio_control_counts_to_user(&db->control_heading.settings,  dif - db->dif_offset);
    *turn_rate = pbio_control_counts_to_user(&db->control_heading.settings,  dif_rate);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_reset_state(pbio_drivebase_t *db) {
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

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
