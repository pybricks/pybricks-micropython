// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdlib.h>

#include <pbdrv/clock.h>
#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

// Drivebase objects

#define NUM_DRIVEBASES (PBDRV_CONFIG_NUM_MOTOR_CONTROLLER / 2)

static pbio_drivebase_t drivebases[NUM_DRIVEBASES];

// The drivebase update can run if both servos are successfully updating
bool pbio_drivebase_update_loop_is_running(pbio_drivebase_t *db) {

    // Drivebase must have servos.
    if (!db->left || !db->right) {
        return false;
    }

    // Drivebase must be the parent of its two servos.
    if (!pbio_parent_equals(&db->left->parent, db) || !pbio_parent_equals(&db->right->parent, db)) {
        return false;
    }

    // Both servo update loops must be running, since we want to read the servo observer state.
    return pbio_servo_update_loop_is_running(db->left) && pbio_servo_update_loop_is_running(db->right);
}

static pbio_error_t drivebase_adopt_settings(pbio_control_settings_t *s_distance, pbio_control_settings_t *s_heading, pbio_control_settings_t *s_left, pbio_control_settings_t *s_right) {

    // All rate/count acceleration limits add up, because distance state is two motors counts added
    s_distance->max_rate = s_left->max_rate + s_right->max_rate;
    s_distance->rate_tolerance = s_left->rate_tolerance + s_right->rate_tolerance;
    s_distance->count_tolerance = s_left->count_tolerance + s_right->count_tolerance;
    s_distance->stall_rate_limit = s_left->stall_rate_limit + s_right->stall_rate_limit;
    s_distance->integral_rate = s_left->integral_rate + s_right->integral_rate;
    s_distance->abs_acceleration = s_left->abs_acceleration + s_right->abs_acceleration;

    // Use the average PID of both motors
    s_distance->pid_kp = (s_left->pid_kp + s_right->pid_kp) / 2;
    s_distance->pid_ki = (s_left->pid_ki + s_right->pid_ki) / 2;
    s_distance->pid_kd = (s_left->pid_kd + s_right->pid_kd) / 2;

    // Maxima are bound by the least capable motor
    s_distance->max_torque = min(s_left->max_torque, s_right->max_torque);
    s_distance->stall_time = min(s_left->stall_time, s_right->stall_time);

    // Copy rate estimator usage, required to be the same on both motors
    if (s_left->use_estimated_rate != s_right->use_estimated_rate) {
        return PBIO_ERROR_INVALID_ARG;
    }
    s_distance->use_estimated_rate = s_left->use_estimated_rate;

    // By default, heading control is the same as distance control
    *s_heading = *s_distance;

    // Allow just slightly more torque for heading. While not technically
    // necessary under nominal circumstances, it gives the expected perceived
    // result of one wheel nearly stopping when you block the other.
    s_heading->max_torque *= 2;

    return PBIO_SUCCESS;
}

// Get the physical and estimated state of a drivebase
static pbio_error_t pbio_drivebase_get_state(pbio_drivebase_t *db, pbio_control_state_t *state_distance, pbio_control_state_t *state_heading) {

    // Get left servo state
    pbio_control_state_t state_left;
    pbio_error_t err = pbio_servo_get_state(db->left, &state_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get right servo state
    pbio_control_state_t state_right;
    err = pbio_servo_get_state(db->right, &state_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Take sum to get distance state
    state_distance->count = state_left.count + state_right.count;
    state_distance->rate = state_left.rate + state_right.rate;

    state_distance->count_est = state_left.count_est + state_right.count_est;
    state_distance->rate_est = state_left.rate_est + state_right.rate_est;

    // Take difference to get heading state
    state_heading->count = state_left.count - state_right.count;
    state_heading->rate = state_left.rate - state_right.rate;

    state_heading->count_est = state_left.count_est - state_right.count_est;
    state_heading->rate_est = state_left.rate_est - state_right.rate_est;

    return PBIO_SUCCESS;
}

static void pbio_drivebase_stop_control(pbio_drivebase_t *db) {
    // Stop control so polling will stop
    pbio_control_stop(&db->control_distance);
    pbio_control_stop(&db->control_heading);
}

// Actuate a drivebase
static pbio_error_t pbio_drivebase_actuate(pbio_drivebase_t *db, pbio_actuation_t actuation, int32_t sum_control, int32_t dif_control) {

    switch (actuation) {
        // Coast and brake are both passed on to servo_actuate as-is.
        case PBIO_ACTUATION_COAST:
        case PBIO_ACTUATION_BRAKE: {
            pbio_drivebase_stop_control(db);
            pbio_error_t err = pbio_servo_actuate(db->left, actuation, 0);
            if (err != PBIO_SUCCESS) {
                return err;
            }
            return pbio_servo_actuate(db->right, actuation, 0);
        }
        // Hold is achieved by driving 0 distance.
        case PBIO_ACTUATION_HOLD:
            return pbio_drivebase_drive_curve(db, 0, 0, db->control_distance.settings.max_rate, db->control_heading.settings.max_rate, PBIO_ACTUATION_HOLD);
        case PBIO_ACTUATION_VOLTAGE:
            return PBIO_ERROR_NOT_IMPLEMENTED;
        case PBIO_ACTUATION_TORQUE:
            return PBIO_ERROR_NOT_IMPLEMENTED;
        default:
            return PBIO_ERROR_INVALID_ARG;
    }
}

// This function is attached to a servo object, so it is able to
// stop the drivebase if the servo needs to execute a new command.
static pbio_error_t pbio_drivebase_stop_from_servo(void *drivebase, bool clear_parent) {

    // Specify pointer type.
    pbio_drivebase_t *db = drivebase;

    // If drive base control is not active, there is nothing we need to do.
    if (!pbio_control_is_active(&db->control_distance) && !pbio_control_is_active(&db->control_heading)) {
        return PBIO_SUCCESS;
    }

    // Stop the drive base controller so the motors don't start moving again.
    pbio_drivebase_stop_control(db);

    // Since we don't know which child called the parent to stop, we stop both
    // motors. We don't stop their parents to avoid escalating the stop calls
    // up the chain (and back here) once again.
    pbio_error_t err = pbio_dcmotor_coast(db->left->dcmotor);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return pbio_dcmotor_coast(db->right->dcmotor);
}

pbio_error_t pbio_drivebase_get_drivebase(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right, fix16_t wheel_diameter, fix16_t axle_track) {

    // Can't build a drive base with just one motor.
    if (left == right) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Assert that both motors have the same gearing
    if (left->control.settings.counts_per_unit != right->control.settings.counts_per_unit) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Check if the servos already have parents.
    if (pbio_parent_exists(&left->parent) || pbio_parent_exists(&right->parent)) {
        // If a servo is already in use by a higher level
        // abstraction like a drivebase, we can't re-use it.
        return PBIO_ERROR_BUSY;
    }

    // Now we know that the servos are free, there must be an available
    // drivebase. We can just use the first one that isn't running.
    uint8_t index;
    for (index = 0; index < NUM_DRIVEBASES; index++) {
        if (!pbio_drivebase_update_loop_is_running(&drivebases[index])) {
            break;
        }
    }
    // Verify result is in range.
    if (index == NUM_DRIVEBASES) {
        return PBIO_ERROR_FAILED;
    }

    // So, this is the drivebase we'll use.
    pbio_drivebase_t *db = &drivebases[index];

    // Set return value.
    *db_address = db;

    // Attach servos
    db->left = left;
    db->right = right;

    // Set parents of both servos, so they can stop this drivebase.
    pbio_parent_set(&left->parent, db, pbio_drivebase_stop_from_servo);
    pbio_parent_set(&right->parent, db, pbio_drivebase_stop_from_servo);

    // Stop any existing drivebase controls
    pbio_drivebase_stop_control(db);

    // Drivebase geometry
    if (wheel_diameter <= 0 || axle_track <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Reset both motors to a passive state
    pbio_error_t err = pbio_drivebase_actuate(db, PBIO_ACTUATION_COAST, 0, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Adopt settings as the average or sum of both servos, except scaling
    err = drivebase_adopt_settings(&db->control_distance.settings, &db->control_heading.settings, &left->control.settings, &right->control.settings);
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

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_actuation_t after_stop) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_drivebase_update_loop_is_running(db)) {
        return PBIO_ERROR_INVALID_OP;
    }

    if (after_stop == PBIO_ACTUATION_HOLD) {

        // Get drive base state
        pbio_control_state_t state_distance;
        pbio_control_state_t state_heading;
        pbio_error_t err = pbio_drivebase_get_state(db, &state_distance, &state_heading);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // When holding, the control payload is the count to hold
        return pbio_drivebase_actuate(db, after_stop, state_distance.count, state_heading.count);

    } else {
        // Otherwise the payload is zero and control stops
        return pbio_drivebase_actuate(db, after_stop, 0, 0);
    }
}

bool pbio_drivebase_is_busy(pbio_drivebase_t *db) {
    return !pbio_control_is_done(&db->control_distance) || !pbio_control_is_done(&db->control_heading);
}

static pbio_error_t pbio_drivebase_update(pbio_drivebase_t *db) {

    // If passive, then exit
    if (db->control_heading.type == PBIO_CONTROL_NONE || db->control_distance.type == PBIO_CONTROL_NONE) {
        return PBIO_SUCCESS;
    }

    // Get current time
    int32_t time_now = pbdrv_clock_get_us();

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get reference and torque signals
    pbio_trajectory_reference_t ref_distance;
    pbio_trajectory_reference_t ref_heading;
    int32_t sum_torque, dif_torque;
    pbio_actuation_t sum_actuation, dif_actuation;
    pbio_control_update(&db->control_distance, time_now, &state_distance, &ref_distance, &sum_actuation, &sum_torque);
    pbio_control_update(&db->control_heading, time_now, &state_heading, &ref_heading, &dif_actuation, &dif_torque);

    // If either controller coasts, coast both, thereby also stopping control.
    if (sum_actuation == PBIO_ACTUATION_COAST || dif_actuation == PBIO_ACTUATION_COAST) {
        return pbio_drivebase_actuate(db, PBIO_ACTUATION_COAST, 0, 0);
    }
    // If either controller brakes, brake both, thereby also stopping control.
    if (sum_actuation == PBIO_ACTUATION_BRAKE || dif_actuation == PBIO_ACTUATION_BRAKE) {
        return pbio_drivebase_actuate(db, PBIO_ACTUATION_BRAKE, 0, 0);
    }

    // The leading controller is able to pause when it stalls. The following controller does not do its own stall,
    // but follows the leader. This ensures they complete at exactly the same time.

    // Check which controller is the follower, if any.
    if (pbio_control_type_is_follower(&db->control_distance)) {
        // Distance control follows, so make it copy heading control pause state
        err = pbio_control_copy_integrator_pause_state(&db->control_heading, &db->control_distance, time_now, state_distance.count, ref_distance.count);
    } else if (pbio_control_type_is_follower(&db->control_heading)) {
        // Heading control follows, so make it copy distance control pause state
        err = pbio_control_copy_integrator_pause_state(&db->control_distance, &db->control_heading, time_now, state_heading.count, ref_heading.count);
    }
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // The left servo drives at a torque and speed of sum / 2 + dif / 2
    int32_t feed_forward_left = pbio_observer_get_feedforward_torque(&db->left->observer, ref_distance.rate / 2 + ref_heading.rate / 2, ref_distance.acceleration / 2 + ref_heading.acceleration / 2);
    err = pbio_servo_actuate(db->left, sum_actuation, sum_torque / 2 + dif_torque / 2 + feed_forward_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // The right servo drives at a torque and speed of sum / 2 - dif / 2
    int32_t feed_forward_right = pbio_observer_get_feedforward_torque(&db->right->observer, ref_distance.rate / 2 - ref_heading.rate / 2, ref_distance.acceleration / 2 - ref_heading.acceleration / 2);
    return pbio_servo_actuate(db->right, dif_actuation, sum_torque / 2 - dif_torque / 2 + feed_forward_right);
}

void pbio_drivebase_update_all(void) {
    // Go through all drive base candidates
    for (uint8_t i = 0; i < NUM_DRIVEBASES; i++) {

        pbio_drivebase_t *db = &drivebases[i];

        // If it's registered for updates, run its update loop
        if (pbio_drivebase_update_loop_is_running(db)) {
            pbio_drivebase_update(db);
        }
    }
}

static pbio_error_t pbio_drivebase_drive_counts_relative(pbio_drivebase_t *db, int32_t sum, int32_t sum_rate, int32_t dif, int32_t dif_rate, pbio_actuation_t after_stop) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_drivebase_update_loop_is_running(db)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get current time
    int32_t time_now = pbdrv_clock_get_us();

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start controller that controls the sum of both motor counts
    err = pbio_control_start_relative_angle_control(&db->control_distance, time_now, &state_distance, sum, sum_rate, after_stop);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start controller that controls the difference between both motor counts
    err = pbio_control_start_relative_angle_control(&db->control_heading, time_now, &state_heading, dif, dif_rate, after_stop);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // At this point, the two trajectories may have different durations, so they won't complete at the same time
    // To account for this, we re-compute the shortest trajectory to have the same duration as the longest.

    // First, find out which controller takes the lead
    pbio_control_t *control_leader;
    pbio_control_t *control_follower;
    if (db->control_distance.trajectory.t3 > db->control_heading.trajectory.t3) {
        // Distance control takes the longest, so it will take the lead
        control_leader = &db->control_distance;
        control_follower = &db->control_heading;
    } else {
        // Heading control takes the longest, so it will take the lead
        control_leader = &db->control_heading;
        control_follower = &db->control_distance;
    }

    // Revise follower trajectory so it takes as long as the leader, achieved
    // by picking a lower speed and accelerations that makes the times match.
    pbio_trajectory_stretch(&control_follower->trajectory, control_leader->trajectory.t1, control_leader->trajectory.t2, control_leader->trajectory.t3);

    // The follower trajector holds until the leader trajectory says otherwise
    control_follower->after_stop = PBIO_ACTUATION_HOLD;
    control_follower->type = PBIO_CONTROL_ANGLE_FOLLOW;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_drive_curve(pbio_drivebase_t *db, int32_t radius, int32_t angle_or_distance, int32_t drive_speed, int32_t turn_rate, pbio_actuation_t after_stop) {

    int32_t arc_angle, arc_length;
    if (radius == PBIO_RADIUS_INF) {
        // For infinite radius, we want to drive straight, and
        // the angle_or_distance input is interpreted as distance.
        arc_angle = 0;
        arc_length = angle_or_distance;
    } else {
        // In the normal case, angle_or_distance is interpreted as the angle,
        // signed by the radius. Arc length is computed accordingly.
        arc_angle = radius < 0 ? -angle_or_distance : angle_or_distance;
        arc_length = (10 * abs(angle_or_distance) * radius) / 573;
    }

    // Convert arc length and speed to motor counts based on drivebase geometry
    int32_t relative_sum = pbio_control_user_to_counts(&db->control_distance.settings, arc_length);
    int32_t sum_rate = pbio_control_user_to_counts(&db->control_distance.settings, drive_speed);

    // Convert arc angle and speed to motor counts based on drivebase geometry
    int32_t relative_dif = pbio_control_user_to_counts(&db->control_heading.settings, arc_angle);
    int32_t dif_rate = pbio_control_user_to_counts(&db->control_heading.settings, turn_rate);

    return pbio_drivebase_drive_counts_relative(db, relative_sum, sum_rate, relative_dif, dif_rate, after_stop);
}

static pbio_error_t pbio_drivebase_drive_counts_timed(pbio_drivebase_t *db, int32_t sum_rate, int32_t dif_rate, int32_t duration, pbio_control_on_target_t stop_func, pbio_actuation_t after_stop) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_drivebase_update_loop_is_running(db)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get current time
    int32_t time_now = pbdrv_clock_get_us();

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Scale duration to microseconds unless it's forever.
    if (duration != DURATION_FOREVER) {
        duration *= US_PER_MS;
    }

    // Initialize both controllers
    err = pbio_control_start_timed_control(&db->control_distance, time_now, &state_distance, duration, sum_rate, stop_func, after_stop);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = pbio_control_start_timed_control(&db->control_heading, time_now, &state_heading, duration, dif_rate, stop_func, after_stop);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_drive_forever(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate) {
    return pbio_drivebase_drive_counts_timed(db,
        pbio_control_user_to_counts(&db->control_distance.settings, speed),
        pbio_control_user_to_counts(&db->control_heading.settings, turn_rate),
        DURATION_FOREVER, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
}

pbio_error_t pbio_drivebase_get_state_user(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate) {

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *distance = pbio_control_counts_to_user(&db->control_distance.settings, state_distance.count);
    *drive_speed = pbio_control_counts_to_user(&db->control_distance.settings, state_distance.rate);
    *angle = pbio_control_counts_to_user(&db->control_heading.settings, state_heading.count);
    *turn_rate = pbio_control_counts_to_user(&db->control_heading.settings, state_heading.rate);
    return PBIO_SUCCESS;
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

#if !PBIO_CONFIG_CONTROL_MINIMAL

// The following functions provide spike-like "tank-drive" controls. These will
// not use the pbio functionality for gearing or reversed orientation. Any
// scaling and flipping happens within the functions below.

// Set up a drive base without drivebase geometry.
pbio_error_t pbio_drivebase_get_spikebase(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right) {
    // Allow only the same type of motor. To find out, just check that the model parameters are the same.
    if (left->observer.settings != right->observer.settings) {
        return PBIO_ERROR_INVALID_PORT;
    }

    return pbio_drivebase_get_drivebase(db_address, left, right, fix16_one, fix16_one);
}

// Drive forever given two motor speeds.
pbio_error_t pbio_spikebase_drive_forever(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right) {
    // Flip left tank motor orientation.
    speed_left = -speed_left;

    // Start driving forever with the given sum and dif rates.
    return pbio_drivebase_drive_counts_timed(db, speed_left + speed_right, speed_left - speed_right, DURATION_FOREVER, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
}

// Drive for a given duration, given two motor speeds.
pbio_error_t pbio_spikebase_drive_time(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t duration, pbio_actuation_t after_stop) {
    // Flip left tank motor orientation.
    speed_left = -speed_left;

    // Start driving forever with the given sum and dif rates.
    return pbio_drivebase_drive_counts_timed(db, speed_left + speed_right, speed_left - speed_right, duration, pbio_control_on_target_time, after_stop);
}

// Drive given two speeds and one angle.
pbio_error_t pbio_spikebase_drive_angle(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t angle, pbio_actuation_t after_stop) {

    // Ignore two zero speeds or zero angle by making drivebase stop.
    if (angle == 0 || (speed_left == 0 && speed_right == 0)) {
        return pbio_drivebase_stop(db, after_stop);
    }

    // In the classic tank drive, we flip the left motor here instead of at the low level.
    speed_left *= -1;

    // Get relative angle, signed by the direction in which we want to go.
    int32_t angle_left = (abs(angle) * 2 * speed_left) / (abs(speed_left) + abs(speed_right));
    int32_t angle_right = (abs(angle) * 2 * speed_right) / (abs(speed_left) + abs(speed_right));

    // Work out the required total and difference angles to achieve this.
    int32_t sum = angle_left + angle_right;
    int32_t dif = angle_left - angle_right;
    int32_t sum_rate = abs(speed_left + speed_right);
    int32_t dif_rate = abs(speed_left - speed_right);

    // If the angle was negative, we need to reverse the result.
    if (angle < 0) {
        sum_rate *= -1;
        dif_rate *= -1;
    }

    // Execute the maneuver.
    return pbio_drivebase_drive_counts_relative(db, sum, sum_rate, dif, dif_rate, after_stop);
}

pbio_error_t pbio_spikebase_steering_to_tank(int32_t speed, int32_t steering, int32_t *speed_left, int32_t *speed_right) {

    // Steering must be bounded
    if (steering < -100 || steering > 100) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Hard coded special cases
    if (steering == 100) {
        // In-place turn to the right.
        *speed_left = speed;
        *speed_right = -speed;
        return PBIO_SUCCESS;
    }
    if (steering == -100) {
        // In-place turn to the left.
        *speed_left = -speed;
        *speed_right = speed;
        return PBIO_SUCCESS;
    }

    // 99 will be what 100 would have been if 100 wasn't a special case.
    if (steering == 99) {
        // 99 is treated as 100, thus turning the left wheel only.
        steering = 100;
    }
    if (steering == -99) {
        // -99 is treated as -100, thus turning the right wheel only.
        steering = -100;
    }

    // Generic case, where one wheel drives slower, given by steering ratio.
    *speed_left = speed;
    *speed_right = speed;

    // Depending on steering direction, one wheel moves slower.
    *(steering > 0 ? speed_right : speed_left) = speed * (100 - abs(steering)) / 100;
    return PBIO_SUCCESS;
}

#endif // !PBIO_CONFIG_CONTROL_MINIMAL

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
