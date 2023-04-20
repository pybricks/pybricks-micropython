// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <stdlib.h>

#include <pbdrv/clock.h>
#include <pbio/error.h>
#include <pbio/drivebase.h>
#include <pbio/int_math.h>
#include <pbio/imu.h>
#include <pbio/servo.h>

#if PBIO_CONFIG_NUM_DRIVEBASES > 0

// Drivebase objects
static pbio_drivebase_t drivebases[PBIO_CONFIG_NUM_DRIVEBASES];

/**
 * Gets the state of the drivebase update loop.
 *
 * This becomes true after a successful call to pbio_drivebase_setup and
 * becomes false when there is an error. Such as when the cable is unplugged.
 *
 * @param [in]  db          The drivebase instance
 * @return                  True if up and running, false if not.
 */
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

/**
 * Sets the drivebase settings based on the left and right motor settings.
 *
 * Sets all settings except ctl_steps_per_app_step. This must be set after
 * calling this function.
 *
 * @param [out] s_distance  Settings of the distance controller.
 * @param [out] s_heading   Settings of the heading controller.
 * @param [in]  s_left      Settings of the left motor controller.
 * @param [in]  s_right     Settings of the right motor controller.
 */
static void drivebase_adopt_settings(pbio_control_settings_t *s_distance, pbio_control_settings_t *s_heading, const pbio_control_settings_t *s_left, const pbio_control_settings_t *s_right) {

    // Use minimum PID of both motors, to avoid overly aggressive control if
    // one of the two motors has much higher PID values. Then scale it such
    // that we use the reduced kp value not just for low errors, but always.
    int32_t pid_kp = pbio_int_math_min(s_left->pid_kp, s_right->pid_kp) *
        pbio_int_math_min(s_left->pid_kp_low_pct, s_right->pid_kp_low_pct) / 100;

    // Cap maximum speed at the most constrained of the two motors.
    int32_t actuation_max = pbio_int_math_min(s_left->actuation_max, s_right->actuation_max);
    int32_t speed_max = pbio_int_math_min(s_left->speed_max, s_right->speed_max);

    // For all settings, take the value of the least powerful motor to ensure
    // that the drivebase can meet the given specs.
    *s_distance = (pbio_control_settings_t) {
        // Must be set immediately after calling the current function.
        .ctl_steps_per_app_step = 0,
        .stall_speed_limit = pbio_int_math_min(s_left->stall_speed_limit, s_right->stall_speed_limit),
        .stall_time = pbio_int_math_min(s_left->stall_time, s_right->stall_time),
        .speed_max = speed_max,
        // The default speed is 40% of the maximum speed.
        .speed_default = speed_max * 10 / 25,
        .speed_tolerance = pbio_int_math_min(s_left->speed_tolerance, s_right->speed_tolerance),
        // To account for reduced kp, adjust the position tolerance so we
        // always apply enough proportional torque to keep moving near the end.
        .position_tolerance = pbio_control_settings_div_by_gain(actuation_max, pid_kp),
        // Make acceleration, deceleration a bit slower for smoother driving.
        .acceleration = pbio_int_math_min(s_left->acceleration, s_right->acceleration) * 3 / 4,
        .deceleration = pbio_int_math_min(s_left->deceleration, s_right->deceleration) * 3 / 4,
        .actuation_max = actuation_max,
        .pid_kp = pid_kp,
        // Dynamic kp reduction is disabled for drivebases. Instead, it uses
        // reduced kp across the board.
        .pid_kp_low_pct = 0,
        .pid_kp_low_error_threshold = 0,
        .pid_kp_low_speed_threshold = 0,
        // Integral control is not necessary since there is no constant external
        // force to overcome that wouldn't be done by proportional control.
        .pid_ki = 0,
        .pid_kd = pbio_int_math_min(s_left->pid_kd, s_right->pid_kd),
        .integral_deadzone = pbio_int_math_max(s_left->integral_deadzone, s_right->integral_deadzone),
        .integral_change_max = pbio_int_math_min(s_left->integral_change_max, s_right->integral_change_max),
        .smart_passive_hold_time = pbio_int_math_max(s_left->smart_passive_hold_time, s_right->smart_passive_hold_time),
    };

    // By default, heading control is the nearly same as distance control.
    *s_heading = *s_distance;

    // We make the default turn speed a bit slower. Given the typical wheel
    // diameter, the wheels are often quite close together, so this
    // compensates by setting it at 33% instead of 40%.
    s_heading->speed_default = s_heading->speed_max / 3;

    // Most users intuitively expect heading control to take priority. When
    // heading controller is completely saturated, this ensures that it "wins"
    // against the distance controller.
    s_heading->actuation_max = s_distance->actuation_max * 2;
}

/**
 * Get the physical and estimated state of a drivebase in units of control.
 *
 * @param [in]  db              The drivebase instance
 * @param [out] state_distance  Physical and estimated state of the distance.
 * @param [out] state_heading   Physical and estimated state of the heading.
 * @return                      Error code.
 */
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

    // Take average to get distance state
    pbio_angle_avg(&state_left.position, &state_right.position, &state_distance->position);
    pbio_angle_avg(&state_left.position_estimate, &state_right.position_estimate, &state_distance->position_estimate);
    state_distance->speed_estimate = (state_left.speed_estimate + state_right.speed_estimate) / 2;
    state_distance->speed = (state_left.speed + state_right.speed) / 2;

    // Take average difference to get heading state, which is implemented as:
    // (left - right) / 2 = (left + right) / 2 - right = avg - right.
    pbio_angle_diff(&state_distance->position, &state_right.position, &state_heading->position);
    pbio_angle_diff(&state_distance->position_estimate, &state_right.position_estimate, &state_heading->position_estimate);
    state_heading->speed_estimate = state_distance->speed_estimate - state_right.speed_estimate;
    state_heading->speed = state_distance->speed - state_right.speed;

    // Optionally use gyro to override the heading source for more accuracy.
    if (db->use_gyro) {
        pbio_imu_get_heading_scaled(&state_heading->position, &state_heading->speed, db->control_heading.settings.ctl_steps_per_app_step);
    }

    return PBIO_SUCCESS;
}

/**
 * Stop the drivebase from updating its controllers.
 *
 * This does not physically stop the motors if they are already moving.
 *
 * @param [in]  db              The drivebase instance
 */
static void pbio_drivebase_stop_drivebase_control(pbio_drivebase_t *db) {
    // Stop drivebase control so polling will stop
    pbio_control_stop(&db->control_distance);
    pbio_control_stop(&db->control_heading);
    db->control_paused = false;
}

/**
 * Stop the motors used by the drivebase from updating its controllers.
 *
 * This does not physically stop the motors if they are already moving.
 *
 * @param [in]  db              The drivebase instance
 */
static void pbio_drivebase_stop_servo_control(pbio_drivebase_t *db) {
    // Stop servo control so polling will stop
    pbio_control_stop(&db->left->control);
    pbio_control_stop(&db->right->control);
}

/**
 * Checks if both drive base controllers are active.
 *
 * @param [in]  drivebase       Pointer to this drivebase instance.
 * @return                      True if heading and distance control are active, else false.
 */
static bool pbio_drivebase_control_is_active(const pbio_drivebase_t *db) {
    return pbio_control_is_active(&db->control_distance) && pbio_control_is_active(&db->control_heading);
}

/**
 * Drivebase stop function that can be called from a servo.
 *
 * When a new command is issued to a servo, the servo calls this to stop the
 * drivebase controller and to stop the other motor physically.
 *
 * @param [in]  drivebase       Void pointer to this drivebase instance.
 * @param [in]  clear_parent    Unused. There is currently no higher
 *                              abstraction than a drivebase.
 * @return                      Error code.
 */
static pbio_error_t pbio_drivebase_stop_from_servo(void *drivebase, bool clear_parent) {

    // A drivebase has no parent, so clear_parent argument is not applicable.
    (void)clear_parent;

    // Specify pointer type.
    pbio_drivebase_t *db = drivebase;

    // If drive base control is not active, there is nothing we need to do.
    if (!pbio_drivebase_control_is_active(db)) {
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

#define ROT_MDEG_OVER_PI (114592) // 360 000 / pi

/**
 * Gets drivebase instance from two servo instances.
 *
 * @param [out] db_address       Drivebase instance if available.
 * @param [in]  left             Left servo instance.
 * @param [in]  right            Right servo instance.
 * @param [in]  wheel_diameter   Wheel diameter in um.
 * @param [in]  axle_track       Distance between wheel-ground contact points in um.
 * @param [in]  use_gyro         Whether to use a gyro for heading (true) or the builtin rotation sensors (false).
 * @return                       Error code.
 */
pbio_error_t pbio_drivebase_get_drivebase(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right, int32_t wheel_diameter, int32_t axle_track, bool use_gyro) {

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

    // Whether to use the gyro for steering and driving straight.
    db->use_gyro = use_gyro;

    // Set parents of both servos, so they can stop this drivebase.
    pbio_parent_set(&left->parent, db, pbio_drivebase_stop_from_servo);
    pbio_parent_set(&right->parent, db, pbio_drivebase_stop_from_servo);

    // Stop any existing drivebase controls
    pbio_control_reset(&db->control_distance);
    pbio_control_reset(&db->control_heading);
    db->control_paused = false;

    // Reset both motors to a passive state
    pbio_drivebase_stop_servo_control(db);
    pbio_error_t err = pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Adopt settings as the average or sum of both servos, except scaling
    drivebase_adopt_settings(&db->control_distance.settings, &db->control_heading.settings, &left->control.settings, &right->control.settings);

    // Verify that the given dimensions are not too small or large to compute
    // a correct result for heading and distance control scale below.
    if (wheel_diameter < 1000 || axle_track < 1000 ||
        left->control.settings.ctl_steps_per_app_step > INT32_MAX / ROT_MDEG_OVER_PI ||
        left->control.settings.ctl_steps_per_app_step > INT32_MAX / axle_track
        ) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Average rotation of the motors for every 1 degree drivebase rotation.
    db->control_heading.settings.ctl_steps_per_app_step =
        left->control.settings.ctl_steps_per_app_step * axle_track / wheel_diameter;

    // Average rotation of the motors for every 1 mm forward.
    db->control_distance.settings.ctl_steps_per_app_step =
        left->control.settings.ctl_steps_per_app_step * ROT_MDEG_OVER_PI / wheel_diameter;


    // Verify that wheel diameter was not so large that scale is now zero.
    if (db->control_distance.settings.ctl_steps_per_app_step < 1 ||
        db->control_heading.settings.ctl_steps_per_app_step < 1) {
        return PBIO_ERROR_INVALID_ARG;
    }

    return PBIO_SUCCESS;
}

/**
 * Stops a drivebase.
 *
 * @param [in]  db               Drivebase instance.
 * @param [in]  on_completion    Which stop type to use.
 * @return                       Error code.
 */
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
    if (on_completion == PBIO_CONTROL_ON_COMPLETION_HOLD) {
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

/**
 * Checks if a drivebase has completed its maneuver.
 *
 * @param [in]  db          The drivebase instance
 * @return                  True if still moving to target, false if not.
 */
bool pbio_drivebase_is_done(const pbio_drivebase_t *db) {
    return pbio_control_is_done(&db->control_distance) && pbio_control_is_done(&db->control_heading);
}

/**
 * Updates one drivebase in the control loop.
 *
 * This reads the physical and estimated state, and updates the controller if
 * it is active.
 *
 * @param [in]  db          The drivebase instance
 * @return                  Error code.
 */
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

    // Get reference and torque signals for distance control.
    pbio_trajectory_reference_t ref_distance;
    int32_t distance_torque;
    pbio_dcmotor_actuation_t distance_actuation;
    bool distance_external_pause = db->control_paused;
    pbio_control_update(&db->control_distance, time_now, &state_distance, &ref_distance, &distance_actuation, &distance_torque, &distance_external_pause);

    // Get reference and torque signals for heading control.
    pbio_trajectory_reference_t ref_heading;
    int32_t heading_torque;
    pbio_dcmotor_actuation_t heading_actuation;
    bool heading_external_pause = db->control_paused;
    pbio_control_update(&db->control_heading, time_now, &state_heading, &ref_heading, &heading_actuation, &heading_torque, &heading_external_pause);

    // If either controller is paused, pause both.
    db->control_paused = distance_external_pause || heading_external_pause;

    // If either controller coasts, coast both, thereby also stopping control.
    if (distance_actuation == PBIO_DCMOTOR_ACTUATION_COAST ||
        heading_actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        return pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_COAST);
    }
    // If either controller brakes, brake both, thereby also stopping control.
    if (distance_actuation == PBIO_DCMOTOR_ACTUATION_BRAKE ||
        heading_actuation == PBIO_DCMOTOR_ACTUATION_BRAKE) {
        return pbio_drivebase_stop(db, PBIO_CONTROL_ON_COMPLETION_BRAKE);
    }

    // The only other expected actuation type is torque, so make sure it is.
    if (distance_actuation != PBIO_DCMOTOR_ACTUATION_TORQUE ||
        heading_actuation != PBIO_DCMOTOR_ACTUATION_TORQUE) {
        return PBIO_ERROR_FAILED;
    }

    // The left servo drives at a torque and speed of (average) + (difference).
    int32_t feed_forward_left = pbio_observer_get_feedforward_torque(
        db->left->observer.model,
        ref_distance.speed + ref_heading.speed, // left speed
        ref_distance.acceleration + ref_heading.acceleration); // left acceleration
    err = pbio_servo_actuate(db->left, PBIO_DCMOTOR_ACTUATION_TORQUE,
        distance_torque + heading_torque + feed_forward_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // The right servo drives at a torque and speed of (average) - (difference).
    int32_t feed_forward_right = pbio_observer_get_feedforward_torque(
        db->right->observer.model,
        ref_distance.speed - ref_heading.speed, // right speed
        ref_distance.acceleration - ref_heading.acceleration); // right acceleration
    return pbio_servo_actuate(db->right, PBIO_DCMOTOR_ACTUATION_TORQUE,
        distance_torque - heading_torque + feed_forward_right);
}

/**
 * Updates all currently active (previously set up) drivebases.
 */
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

/**
 * Starts the drivebase controllers to run by a given distance and angle.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  distance        The distance to run by in mm.
 * @param [in]  drive_speed     The drive speed in mm/s.
 * @param [in]  angle           The angle to turn in deg.
 * @param [in]  turn_speed      The turn speed in deg/s.
 * @param [in]  on_completion   What to do when reaching the target.
 * @return                      Error code.
 */
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

    // Start controller that controls the average angle of both motors.
    err = pbio_control_start_position_control_relative(&db->control_distance, time_now, &state_distance, distance, drive_speed, on_completion, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start controller that controls half the difference between both angles.
    err = pbio_control_start_position_control_relative(&db->control_heading, time_now, &state_heading, angle, turn_speed, on_completion, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // At this point, the two trajectories may have different durations, so they won't complete at the same time
    // To account for this, we re-compute the shortest trajectory to have the same duration as the longest.

    // First, find out which controller takes the lead
    const pbio_control_t *control_leader;
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

/**
 * Starts the drivebase controllers to run by a given distance.
 *
 * This will use the default speed.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  distance        The distance to run by in mm.
 * @param [in]  on_completion   What to do when reaching the target.
 * @return                      Error code.
 */
pbio_error_t pbio_drivebase_drive_straight(pbio_drivebase_t *db, int32_t distance, pbio_control_on_completion_t on_completion) {
    // Execute the common drive command at default speed (by passing 0 speed).
    return pbio_drivebase_drive_relative(db, distance, 0, 0, 0, on_completion);
}

/**
 * Starts the drivebase controllers to run by an arc of given radius and angle.
 *
 * This will use the default speed.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  radius          Radius of the arc in mm.
 * @param [in]  angle           Angle in degrees.
 * @param [in]  on_completion   What to do when reaching the target.
 * @return                      Error code.
 */
pbio_error_t pbio_drivebase_drive_curve(pbio_drivebase_t *db, int32_t radius, int32_t angle, pbio_control_on_completion_t on_completion) {

    // The angle is signed by the radius so we can go both ways.
    int32_t arc_angle = radius < 0 ? -angle : angle;

    // Arc length is computed accordingly.
    int32_t arc_length = (10 * pbio_int_math_abs(angle) * radius) / 573;

    // Execute the common drive command at default speed (by passing 0 speed).
    return pbio_drivebase_drive_relative(db, arc_length, 0, arc_angle, 0, on_completion);
}

/**
 * Starts the drivebase controllers to run for a given duration.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  drive_speed     The drive speed in mm/s.
 * @param [in]  turn_speed      The turn speed in deg/s.
 * @param [in]  duration        The duration in ms.
 * @param [in]  on_completion   What to do when reaching the target.
 * @return                      Error code.
 */
static pbio_error_t pbio_drivebase_drive_time_common(pbio_drivebase_t *db, int32_t drive_speed, int32_t turn_speed, uint32_t duration, pbio_control_on_completion_t on_completion) {

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

/**
 * Starts the drivebase controllers to run forever.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  speed           The drive speed in mm/s.
 * @param [in]  turn_rate       The turn rate in deg/s.
 * @return                      Error code.
 */
pbio_error_t pbio_drivebase_drive_forever(pbio_drivebase_t *db, int32_t speed, int32_t turn_rate) {
    return pbio_drivebase_drive_time_common(db, speed, turn_rate, PBIO_TRAJECTORY_DURATION_FOREVER_MS, PBIO_CONTROL_ON_COMPLETION_CONTINUE);
}

/**
 * Gets the drivebase state in user units.
 *
 * @param [in]  db          The drivebase instance.
 * @param [out] distance    Distance traveled in mm.
 * @param [out] drive_speed Current speed in mm/s.
 * @param [out] angle       Angle turned in degrees.
 * @param [out] turn_rate   Current turn rate in deg/s.
 * @return                  Error code.
 */
pbio_error_t pbio_drivebase_get_state_user(pbio_drivebase_t *db, int32_t *distance, int32_t *drive_speed, int32_t *angle, int32_t *turn_rate) {

    // Get drive base state
    pbio_control_state_t state_distance;
    pbio_control_state_t state_heading;
    pbio_error_t err = pbio_drivebase_get_state_control(db, &state_distance, &state_heading);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    *distance = pbio_control_settings_ctl_to_app_long(&db->control_distance.settings, &state_distance.position);
    *drive_speed = pbio_control_settings_ctl_to_app(&db->control_distance.settings, state_distance.speed);
    *angle = pbio_control_settings_ctl_to_app_long(&db->control_heading.settings, &state_heading.position);
    *turn_rate = pbio_control_settings_ctl_to_app(&db->control_heading.settings, state_heading.speed);
    return PBIO_SUCCESS;
}


/**
 * Gets the drivebase settings in user units.
 *
 * @param [in]  db                  Drivebase instance.
 * @param [out] drive_speed         Default linear speed in mm/s.
 * @param [out] drive_acceleration  Linear acceleration in mm/s^2.
 * @param [out] drive_deceleration  Linear deceleration in mm/s^2.
 * @param [out] turn_rate           Default turn rate in deg/s.
 * @param [out] turn_acceleration   Angular acceleration in deg/s^2.
 * @param [out] turn_deceleration   Angular deceleration in deg/s^2.
 */
pbio_error_t pbio_drivebase_get_drive_settings(const pbio_drivebase_t *db, int32_t *drive_speed, int32_t *drive_acceleration, int32_t *drive_deceleration, int32_t *turn_rate, int32_t *turn_acceleration, int32_t *turn_deceleration) {

    const pbio_control_settings_t *sd = &db->control_distance.settings;
    const pbio_control_settings_t *sh = &db->control_heading.settings;

    *drive_speed = pbio_control_settings_ctl_to_app(sd, sd->speed_default);
    *drive_acceleration = pbio_control_settings_ctl_to_app(sd, sd->acceleration);
    *drive_deceleration = pbio_control_settings_ctl_to_app(sd, sd->deceleration);
    *turn_rate = pbio_control_settings_ctl_to_app(sh, sh->speed_default);
    *turn_acceleration = pbio_control_settings_ctl_to_app(sh, sh->acceleration);
    *turn_deceleration = pbio_control_settings_ctl_to_app(sh, sh->deceleration);

    return PBIO_SUCCESS;
}

/**
 * Sets the drivebase settings in user units.
 *
 * @param [in]  db                  Drivebase instance.
 * @param [in] drive_speed          Default linear speed in mm/s.
 * @param [in] drive_acceleration   Linear acceleration in mm/s^2.
 * @param [in] drive_deceleration   Linear deceleration in mm/s^2.
 * @param [in] turn_rate            Default turn rate in deg/s.
 * @param [in] turn_acceleration    Angular acceleration in deg/s^2.
 * @param [in] turn_deceleration    Angular deceleration in deg/s^2.
 */
pbio_error_t pbio_drivebase_set_drive_settings(pbio_drivebase_t *db, int32_t drive_speed, int32_t drive_acceleration, int32_t drive_deceleration, int32_t turn_rate, int32_t turn_acceleration, int32_t turn_deceleration) {

    pbio_control_settings_t *sd = &db->control_distance.settings;
    pbio_control_settings_t *sh = &db->control_heading.settings;

    pbio_error_t err = pbio_trajectory_validate_speed_limit(sd->ctl_steps_per_app_step, drive_speed);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_trajectory_validate_acceleration_limit(sd->ctl_steps_per_app_step, drive_acceleration);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_trajectory_validate_acceleration_limit(sd->ctl_steps_per_app_step, drive_deceleration);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_trajectory_validate_speed_limit(sh->ctl_steps_per_app_step, turn_rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_trajectory_validate_acceleration_limit(sh->ctl_steps_per_app_step, turn_acceleration);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_trajectory_validate_acceleration_limit(sh->ctl_steps_per_app_step, turn_deceleration);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    sd->speed_default = pbio_int_math_clamp(pbio_control_settings_app_to_ctl(sd, drive_speed), sd->speed_max);
    sd->acceleration = pbio_control_settings_app_to_ctl(sd, drive_acceleration);
    sd->deceleration = pbio_control_settings_app_to_ctl(sd, drive_deceleration);
    sh->speed_default = pbio_int_math_clamp(pbio_control_settings_app_to_ctl(sh, turn_rate), sh->speed_max);
    sh->acceleration = pbio_control_settings_app_to_ctl(sh, turn_acceleration);
    sh->deceleration = pbio_control_settings_app_to_ctl(sh, turn_deceleration);

    return PBIO_SUCCESS;
}

/**
 * Checks whether drivebase is stalled. If the drivebase is actively
 * controlled, it is stalled when the controller(s) cannot maintain the
 * target speed or position while using maximum allowed torque. If control
 * is not active, it uses the individual servos to check for stall.
 *
 * @param [in]  db              The servo instance.
 * @param [out] stalled         True if stalled, false if not.
 * @param [out] stall_duration  For how long it has been stalled (ms).
 * @return                      Error code. ::PBIO_ERROR_INVALID_OP if update
 *                              loop not running, else ::PBIO_SUCCESS
 */
pbio_error_t pbio_drivebase_is_stalled(pbio_drivebase_t *db, bool *stalled, uint32_t *stall_duration) {

    // Don't allow access if update loop not registered.
    if (!pbio_drivebase_update_loop_is_running(db)) {
        *stalled = false;
        *stall_duration = 0;
        return PBIO_ERROR_INVALID_OP;
    }

    pbio_error_t err;

    // If drive base control is active, look at controller state.
    if (pbio_drivebase_control_is_active(db)) {
        uint32_t stall_duration_distance; // ticks, 0 on false
        uint32_t stall_duration_heading; // ticks, 0 on false
        bool stalled_heading = pbio_control_is_stalled(&db->control_heading, &stall_duration_heading);
        bool stalled_distance = pbio_control_is_stalled(&db->control_distance, &stall_duration_distance);

        // We are stalled if any controller is stalled.
        *stalled = stalled_heading || stalled_distance;
        *stall_duration = pbio_control_time_ticks_to_ms(pbio_int_math_max(stall_duration_distance, stall_duration_heading));
        return PBIO_SUCCESS;
    }

    // Otherwise look at individual servos.
    bool stalled_left;
    bool stalled_right;
    uint32_t stall_duration_left; // ms, 0 on false.
    uint32_t stall_duration_right; // ms, 0 on false.
    err = pbio_servo_is_stalled(db->left, &stalled_left, &stall_duration_left);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_is_stalled(db->left, &stalled_right, &stall_duration_right);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // We are stalled if at least one motor is stalled.
    *stalled = stalled_left || stalled_right;
    *stall_duration = pbio_int_math_max(stall_duration_left, stall_duration_right);
    return PBIO_SUCCESS;
}

#if PBIO_CONFIG_DRIVEBASE_SPIKE

/**
 * Gets spike drivebase instance from two servo instances.
 *
 * This and the following functions provide spike-like "tank-drive" controls.
 * These will not use the pbio functionality for gearing or reversed
 * orientation. Any scaling and flipping happens within the functions below.
 *
 * It is a drivebase, but without wheel size and axle track parameters.
 * Instead, the internal conversions are such that the "distance" is expressed
 * in motor degrees. Likewise, turning in place by N degrees means that both
 * motors turn by that amount.
 *
 * @param [out] db_address       Drivebase instance if available.
 * @param [in]  left             Left servo instance.
 * @param [in]  right            Right servo instance.
 * @return                       Error code.
 */
pbio_error_t pbio_drivebase_get_drivebase_spike(pbio_drivebase_t **db_address, pbio_servo_t *left, pbio_servo_t *right) {
    pbio_error_t err = pbio_drivebase_get_drivebase(db_address, left, right, 1000, 1000, false);

    // The application input for spike bases is degrees per second average
    // between both wheels, so in millidegrees this is x1000.
    (*db_address)->control_heading.settings.ctl_steps_per_app_step = 1000;
    (*db_address)->control_distance.settings.ctl_steps_per_app_step = 1000;
    return err;
}

/**
 * Starts driving for a given duration, at the provided motor speeds.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  speed_left      Left motor speed in deg/s.
 * @param [in]  speed_right     Right motor speed in deg/s.
 * @param [in]  duration        The duration in ms.
 * @param [in]  on_completion   What to do when reaching the target.
 * @return                      Error code.
 */
pbio_error_t pbio_drivebase_spike_drive_time(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right, uint32_t duration, pbio_control_on_completion_t on_completion) {
    // Flip left tank motor orientation.
    speed_left = -speed_left;

    // Start driving forever with the given sum and dif rates.
    int32_t drive_speed = (speed_left + speed_right) / 2;
    int32_t turn_speed = (speed_left - speed_right) / 2;
    return pbio_drivebase_drive_time_common(db, drive_speed, turn_speed, duration, on_completion);
}

/**
 * Starts driving indefinitely, at the provided motor speeds.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  speed_left      Left motor speed in deg/s.
 * @param [in]  speed_right     Right motor speed in deg/s.
 * @return                      Error code.
 */
pbio_error_t pbio_drivebase_spike_drive_forever(pbio_drivebase_t *db, int32_t speed_left, int32_t speed_right) {
    // Same as driving for time, just without an endpoint.
    return pbio_drivebase_spike_drive_time(db, speed_left, speed_right, PBIO_TRAJECTORY_DURATION_FOREVER_MS, PBIO_CONTROL_ON_COMPLETION_CONTINUE);
}

/**
 * Drive the motors by a given angle, at the provided motor speeds.
 *
 * Only the faster motor will travel by the given angle. The slower motor
 * travels less, such that they still stop at the same time.
 *
 * @param [in]  db              The drivebase instance.
 * @param [in]  speed_left      Left motor speed in deg/s.
 * @param [in]  speed_right     Right motor speed in deg/s.
 * @param [in]  angle           Angle (deg) that the fast motor should travel.
 * @param [in]  on_completion   What to do when reaching the target.
 * @return                      Error code.
 */
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
    int32_t max_speed = pbio_int_math_max(pbio_int_math_abs(speed_left), pbio_int_math_abs(speed_right));
    int32_t angle_left = max_speed == 0 ? 0 : angle * speed_left / max_speed;
    int32_t angle_right = max_speed == 0 ? 0 : angle * speed_right / max_speed;

    // Work out the required total and difference angles to achieve this.
    int32_t distance = (angle_left + angle_right) / 2;
    int32_t turn_angle = (angle_left - angle_right) / 2;
    int32_t speed = (pbio_int_math_abs(speed_left) + pbio_int_math_abs(speed_right)) / 2;

    // Execute the maneuver.
    return pbio_drivebase_drive_relative(db, distance, speed, turn_angle, speed, on_completion);
}

/**
 * Converts a speed and a steering ratio into a separate left and right speed.
 *
 * The steering value must be in the range [-100, 100].
 *
 * @param [in]  speed         Overall speed (deg/s).
 * @param [in]  steering      Steering ratio.
 * @param [out] speed_left    Speed of the left motor (deg/s).
 * @param [out] speed_right   Speed of the right motor (deg/s).
 * @return                    Error code.
 */
pbio_error_t pbio_drivebase_spike_steering_to_tank(int32_t speed, int32_t steering, int32_t *speed_left, int32_t *speed_right) {

    // Steering must be bounded.
    if (steering < -100 || steering > 100) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Initialize both at the given speed.
    *speed_left = speed;
    *speed_right = speed;

    // Depending on steering direction, one wheel moves slower.
    *(steering > 0 ? speed_right : speed_left) = speed * (100 - 2 * pbio_int_math_abs(steering)) / 100;
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_DRIVEBASE_SPIKE

#endif // PBIO_CONFIG_NUM_DRIVEBASES > 0
