// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdlib.h>

#include <pbdrv/clock.h>
#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/math.h>
#include <pbio/servo.h>

#if PBIO_CONFIG_NUM_DRIVEBASES > 0

// Drivebase objects

static pbio_drivebase_t drivebases[PBIO_CONFIG_NUM_DRIVEBASES];

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

    // For all settings, take the value of the least powerful motor to ensure
    // that the drivebase can meet the given specs.
    s_distance->speed_max = pbio_math_min(s_left->speed_max, s_right->speed_max);
    s_distance->speed_tolerance = pbio_math_min(s_left->speed_tolerance, s_right->speed_tolerance);
    s_distance->stall_speed_limit = pbio_math_min(s_left->stall_speed_limit, s_right->stall_speed_limit);
    s_distance->integral_change_max = pbio_math_min(s_left->integral_change_max, s_right->integral_change_max);
    s_distance->actuation_max = pbio_math_min(s_left->actuation_max, s_right->actuation_max);
    s_distance->stall_time = pbio_math_min(s_left->stall_time, s_right->stall_time);

    // Make acceleration a bit slower for smoother driving.
    s_distance->acceleration = pbio_math_min(s_left->acceleration, s_right->acceleration) * 3 / 4;
    s_distance->deceleration = pbio_math_min(s_left->deceleration, s_right->deceleration) * 3 / 4;

    // Use minimum PID of both motors, to avoid overly aggressive control if
    // one of the two motors has much higher PID values. For proportional
    // control, take a much lower gain. Drivebases don't need it, and it makes
    // for a smoother ride.
    s_distance->pid_kp = pbio_math_min(s_left->pid_kp, s_right->pid_kp) / 4;
    s_distance->pid_kd = pbio_math_min(s_left->pid_kd, s_right->pid_kd);

    // Integral control is not necessary since there is constant external
    // force to overcome that wouldn't be done by proportional control.
    s_distance->pid_ki = 0;

    // Instead, we can adjust the position tolerance to ensure we always apply
    // enough proportional torque to keep moving near the end.
    s_distance->position_tolerance = s_distance->actuation_max / s_distance->pid_kp * 1000;

    // The default speed is 40% of the maximum speed.
    s_distance->speed_default = s_distance->speed_max * 10 / 25;

    // By default, heading control is the nearly same as distance control.
    *s_heading = *s_distance;

    // We make the default turn speed a bit slower. Given the typical wheel
    // diameter, the wheels are often quite close together, so this compensates.
    s_heading->speed_default = s_heading->speed_max / 3;
    return PBIO_SUCCESS;
}

// Get the physical and estimated state of a drivebase
static pbio_error_t pbio_drivebase_get_state_control(pbio_drivebase_t *db, pbio_control_state_t *state_distance, pbio_control_state_t *state_heading) {

    // Get left servo state
    pbio_control_state_t state_left;
    pbio_error_t err = pbio_servo_get_state_control(db->left, &state_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get right servo state
    pbio_control_state_t state_right;
    err = pbio_servo_get_state_control(db->right, &state_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Take sum to get distance state
    pbio_angle_avg(&state_left.position, &state_right.position, &state_distance->position);
    pbio_angle_avg(&state_left.position_estimate, &state_right.position_estimate, &state_distance->position_estimate);
    state_distance->speed_estimate = (state_left.speed_estimate + state_right.speed_estimate) / 2;

    // Take difference to get heading state, which is implemented as
    // (left - right) / 2 = (left + right) / 2 - right = avg - right.
    pbio_angle_diff(&state_distance->position, &state_right.position, &state_heading->position);
    pbio_angle_diff(&state_distance->position_estimate, &state_right.position_estimate, &state_heading->position_estimate);
    state_heading->speed_estimate = state_distance->speed_estimate - state_right.speed_estimate;

    return PBIO_SUCCESS;
}

static void pbio_drivebase_stop_drivebase_control(pbio_drivebase_t *db) {
    // Stop drivebase control so polling will stop
    pbio_control_stop(&db->control_distance);
    pbio_control_stop(&db->control_heading);
}

static void pbio_drivebase_stop_servo_control(pbio_drivebase_t *db) {
    // Stop servo control so polling will stop
    pbio_control_stop(&db->left->control);
    pbio_control_stop(&db->right->control);
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
    pbio_drivebase_stop_drivebase_control(db);

    // Since we don't know which child called the parent to stop, we stop both
    // motors. We don't stop their parents to avoid escalating the stop calls
    // up the chain (and back here) once again.
    pbio_error_t err = pbio_dcmotor_coast(db->left->dcmotor);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return pbio_dcmotor_coast(db->right->dcmotor);
}

pbio_error_t pbio_drivebase_get_drivebase(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right, int32_t wheel_diameter, int32_t axle_track) {

    // Can't build a drive base with just one motor.
    if (left == right) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Assert that both motors have the same gearing
    if (left->control.settings.ctl_steps_per_app_step != right->control.settings.ctl_steps_per_app_step) {
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
    for (index = 0; index < PBIO_CONFIG_NUM_DRIVEBASES; index++) {
        if (!pbio_drivebase_update_loop_is_running(&drivebases[index])) {
            break;
        }
    }
    // Verify result is in range.
    if (index == PBIO_CONFIG_NUM_DRIVEBASES) {
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
    pbio_control_reset(&db->control_distance);
    pbio_control_reset(&db->control_heading);

    // Drivebase geometry
    if (wheel_diameter <= 0 || axle_track <= 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Reset both motors to a passive state
    pbio_drivebase_stop_servo_control(db);
    pbio_error_t err = pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Adopt settings as the average or sum of both servos, except scaling
    err = drivebase_adopt_settings(&db->control_distance.settings, &db->control_heading.settings, &left->control.settings, &right->control.settings);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Average rotation of the motors for every 1 degree drivebase rotation.
    db->control_heading.settings.ctl_steps_per_app_step =
        left->control.settings.ctl_steps_per_app_step * axle_track / wheel_diameter;

    // Average rotation of the motors for every 1 mm forward.
    db->control_distance.settings.ctl_steps_per_app_step =
        left->control.settings.ctl_steps_per_app_step * 2292 /
        wheel_diameter / 20;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_stop(pbio_drivebase_t *db, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_drivebase_update_loop_is_running(db)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // We're asked to stop, so continuing makes no sense.
    if (on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Holding is the same as traveling by 0 degrees.
    if (on_completion == PBIO_CONTROL_ON_COMPLETION_CONTINUE) {
        return pbio_drivebase_drive_straight(db, 0, on_completion);
    }

    // Stop control.
    pbio_drivebase_stop_drivebase_control(db);

    // Stop the servos and pass on requested stop type.
    pbio_error_t err = pbio_servo_stop(db->left, on_completion);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return pbio_servo_stop(db->right, on_completion);
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
    uint32_t time_now = pbio_control_get_time_ticks();

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state_control(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get reference and torque signals
    pbio_trajectory_reference_t ref_distance;
    pbio_trajectory_reference_t ref_heading;
    int32_t sum_torque, dif_torque;
    pbio_dcmotor_actuation_t sum_actuation, dif_actuation;
    pbio_control_update(&db->control_distance, time_now, &state_distance, &ref_distance, &sum_actuation, &sum_torque);
    pbio_control_update(&db->control_heading, time_now, &state_heading, &ref_heading, &dif_actuation, &dif_torque);

    // If either controller coasts, coast both, thereby also stopping control.
    if (sum_actuation == PBIO_DCMOTOR_ACTUATION_COAST ||
        dif_actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        return pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_COAST);
    }
    // If either controller brakes, brake both, thereby also stopping control.
    if (sum_actuation == PBIO_DCMOTOR_ACTUATION_BRAKE ||
        dif_actuation == PBIO_DCMOTOR_ACTUATION_BRAKE) {
        return pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_BRAKE);
    }

    // Both controllers are able to stop the other when it stalls. This ensures
    // they complete at exactly the same time.
    if (pbio_control_type_is_position(&db->control_distance) && !db->control_distance.position_integrator.trajectory_running) {
        pbio_position_integrator_pause(&db->control_heading.position_integrator, time_now);
    }
    if (pbio_control_type_is_position(&db->control_heading) && !db->control_heading.position_integrator.trajectory_running) {
        pbio_position_integrator_pause(&db->control_distance.position_integrator, time_now);
    }

    // The left servo drives at a torque and speed of sum + dif
    int32_t feed_forward_left = pbio_observer_get_feedforward_torque(db->left->observer.model, ref_distance.speed + ref_heading.speed, ref_distance.acceleration + ref_heading.acceleration);
    err = pbio_servo_actuate(db->left, sum_actuation, sum_torque + dif_torque + feed_forward_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // The right servo drives at a torque and speed of sum - dif
    int32_t feed_forward_right = pbio_observer_get_feedforward_torque(db->right->observer.model, ref_distance.speed - ref_heading.speed, ref_distance.acceleration - ref_heading.acceleration);
    return pbio_servo_actuate(db->right, dif_actuation, sum_torque - dif_torque + feed_forward_right);
}

void pbio_drivebase_update_all(void) {
    // Go through all drive base candidates
    for (uint8_t i = 0; i < PBIO_CONFIG_NUM_DRIVEBASES; i++) {

        pbio_drivebase_t *db = &drivebases[i];

        // If it's registered for updates, run its update loop
        if (pbio_drivebase_update_loop_is_running(db)) {
            pbio_drivebase_update(db);
        }
    }
}

static pbio_error_t pbio_drivebase_drive_relative(pbio_drivebase_t *db, int32_t distance, int32_t drive_speed, int32_t angle, int32_t turn_speed, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_drivebase_update_loop_is_running(db)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop servo control in case it was running.
    pbio_drivebase_stop_servo_control(db);

    // Get current time
    uint32_t time_now = pbio_control_get_time_ticks();

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state_control(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start controller that controls the sum of both motor counts
    err = pbio_control_start_position_control_relative(&db->control_distance, time_now, &state_distance, distance, drive_speed, on_completion);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start controller that controls the difference between both motor counts
    err = pbio_control_start_position_control_relative(&db->control_heading, time_now, &state_heading, angle, turn_speed, on_completion);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // At this point, the two trajectories may have different durations, so they won't complete at the same time
    // To account for this, we re-compute the shortest trajectory to have the same duration as the longest.

    // First, find out which controller takes the lead
    pbio_control_t *control_leader;
    pbio_control_t *control_follower;

    if (pbio_trajectory_get_duration(&db->control_distance.trajectory) >
        pbio_trajectory_get_duration(&db->control_heading.trajectory)) {
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
    pbio_trajectory_stretch(&control_follower->trajectory, &control_leader->trajectory);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_drive_straight(pbio_drivebase_t *db, int32_t distance, pbio_control_on_completion_t on_completion) {
    // Execute the common drive command at default speed.
    return pbio_drivebase_drive_relative(db, distance, 0, 0, 0, on_completion);
}
pbio_error_t pbio_drivebase_drive_curve(pbio_drivebase_t *db, int32_t radius, int32_t angle, pbio_control_on_completion_t on_completion) {

    // The angle is signed by the radius so we can go both ways.
    int32_t arc_angle = radius < 0 ? -angle : angle;

    // Arc length is computed accordingly.
    int32_t arc_length = (10 * pbio_math_abs(angle) * radius) / 573;

    // Execute the common drive command at default speed.
    return pbio_drivebase_drive_relative(db, arc_length, 0, arc_angle, 0, on_completion);
}

static pbio_error_t pbio_drivebase_drive_timed(pbio_drivebase_t *db, int32_t drive_speed, int32_t turn_speed, uint32_t duration, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_drivebase_update_loop_is_running(db)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop servo control in case it was running.
    pbio_drivebase_stop_servo_control(db);

    // Get current time
    uint32_t time_now = pbio_control_get_time_ticks();

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state_control(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Initialize both controllers
    err = pbio_control_start_timed_control(&db->control_distance, time_now, &state_distance, duration, drive_speed, on_completion);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    err = pbio_control_start_timed_control(&db->control_heading, time_now, &state_heading, duration, turn_speed, on_completion);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_drive_forever(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate) {
    return pbio_drivebase_drive_timed(db, speed, turn_rate, DURATION_FOREVER_TICKS, PBIO_CONTROL_ON_COMPLETION_CONTINUE);
}

pbio_error_t pbio_drivebase_get_state_user(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate) {

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state_control(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *distance = pbio_control_settings_ctl_to_app_long(&db->control_distance.settings, &state_distance.position);
    *drive_speed = pbio_control_settings_ctl_to_app(&db->control_distance.settings, state_distance.speed_estimate);
    *angle = pbio_control_settings_ctl_to_app_long(&db->control_heading.settings, &state_heading.position);
    *turn_rate = pbio_control_settings_ctl_to_app(&db->control_heading.settings, state_heading.speed_estimate);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_get_drive_settings(pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *drive_deceleration, int32_t *turn_rate, int32_t *turn_acceleration, int32_t *turn_deceleration) {

    pbio_control_settings_t *sd = &db->control_distance.settings;
    pbio_control_settings_t *sh = &db->control_heading.settings;

    *drive_speed = pbio_control_settings_ctl_to_app(sd, sd->speed_default);
    *drive_acceleration = pbio_control_settings_ctl_to_app(sd, sd->acceleration);
    *drive_deceleration = pbio_control_settings_ctl_to_app(sd, sd->deceleration);
    *turn_rate = pbio_control_settings_ctl_to_app(sh, sh->speed_default);
    *turn_acceleration = pbio_control_settings_ctl_to_app(sh, sh->acceleration);
    *turn_deceleration = pbio_control_settings_ctl_to_app(sh, sh->deceleration);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t drive_deceleration, int32_t turn_rate, int32_t turn_acceleration, int32_t turn_deceleration) {

    pbio_control_settings_t *sd = &db->control_distance.settings;
    pbio_control_settings_t *sh = &db->control_heading.settings;

    sd->speed_default = pbio_math_clamp(pbio_control_settings_app_to_ctl(sd, drive_speed), sd->speed_max);
    sd->acceleration = pbio_control_settings_app_to_ctl(sd, drive_acceleration);
    sd->deceleration = pbio_control_settings_app_to_ctl(sd, drive_deceleration);
    sh->speed_default = pbio_math_clamp(pbio_control_settings_app_to_ctl(sh, turn_rate), sh->speed_max);
    sh->acceleration = pbio_control_settings_app_to_ctl(sh, turn_acceleration);
    sh->deceleration = pbio_control_settings_app_to_ctl(sh, turn_deceleration);

    return PBIO_SUCCESS;
}

#if PBIO_CONFIG_DRIVEBASE_SPIKE

// The following functions provide spike-like "tank-drive" controls. These will
// not use the pbio functionality for gearing or reversed orientation. Any
// scaling and flipping happens within the functions below.

// Set up a drive base without drivebase geometry.
pbio_error_t pbio_drivebase_get_drivebase_spike(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right) {
    pbio_error_t err = pbio_drivebase_get_drivebase(db_address, left, right, 1, 1);

    // The application input for spike bases is degrees per second average
    // between both wheels, so in millidegrees this is x1000.
    (*db_address)->control_heading.settings.ctl_steps_per_app_step = 1000;
    (*db_address)->control_distance.settings.ctl_steps_per_app_step = 1000;
    return err;
}

// Drive for a given duration, given two motor speeds.
pbio_error_t pbio_drivebase_spike_drive_time(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t duration, pbio_control_on_completion_t on_completion) {
    // Flip left tank motor orientation.
    speed_left = -speed_left;

    // Start driving forever with the given sum and dif rates.
    int32_t drive_speed = (speed_left + speed_right) / 2;
    int32_t turn_speed = (speed_left - speed_right) / 2;
    return pbio_drivebase_drive_timed(db, drive_speed, turn_speed, pbio_control_time_ms_to_ticks(duration), on_completion);
}

// Drive forever given two motor speeds.
pbio_error_t pbio_drivebase_spike_drive_forever(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right) {
    // Same as driving for time, just without an endpoint.
    return pbio_drivebase_spike_drive_time(db, speed_left, speed_right, DURATION_FOREVER_TICKS, PBIO_CONTROL_ON_COMPLETION_CONTINUE);
}

// Drive given two speeds and one angle.
pbio_error_t pbio_drivebase_spike_drive_angle(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, int32_t angle, pbio_control_on_completion_t on_completion) {

    // In the classic tank drive, we flip the left motor here instead of at the low level.
    speed_left *= -1;

    // If both speeds are zero, we can't do anything meaningful, but most users
    // find it confusing if we return an error. To make sure it won't block
    // forever, we set the angle to zero instead, so we're "done" right away.
    if (speed_left == 0 && speed_right == 0) {
        return pbio_drivebase_drive_relative(db, 0, 0, 0, 0, on_completion);
    }

    // Work out angles for each motor.
    int32_t max_speed = pbio_math_max(pbio_math_abs(speed_left), pbio_math_abs(speed_right));
    int32_t angle_left = max_speed == 0 ? 0 : angle * speed_left / max_speed;
    int32_t angle_right = max_speed == 0 ? 0 : angle * speed_right / max_speed;

    // Work out the required total and difference angles to achieve this.
    int32_t distance = (angle_left + angle_right) / 2;
    int32_t turn_angle = (angle_left - angle_right) / 2;
    int32_t speed = (pbio_math_abs(speed_left) + pbio_math_abs(speed_right)) / 2;

    // Execute the maneuver.
    return pbio_drivebase_drive_relative(db, distance, speed, turn_angle, speed, on_completion);
}

pbio_error_t pbio_drivebase_spike_steering_to_tank(int32_t speed, int32_t steering, int32_t *speed_left, int32_t *speed_right) {

    // Steering must be bounded.
    if (steering < -100 || steering > 100) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Initialize both at the given speed.
    *speed_left = speed;
    *speed_right = speed;

    // Depending on steering direction, one wheel moves slower.
    *(steering > 0 ? speed_right : speed_left) = speed * (100 - 2 * pbio_math_abs(steering)) / 100;
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_DRIVEBASE_SPIKE

#endif // PBIO_CONFIG_NUM_DRIVEBASES > 0
