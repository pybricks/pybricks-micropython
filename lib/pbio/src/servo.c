// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <pbdrv/clock.h>
#include <pbdrv/counter.h>

#include <pbio/math.h>
#include <pbio/observer.h>
#include <pbio/parent.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

// Servo motor objects
static pbio_servo_t servos[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

pbio_error_t pbio_servo_get_servo(pbio_port_id_t port, pbio_servo_t **srv) {

    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get address of static servo object.
    *srv = &servos[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    // Get dcmotor object, without additional setup.
    pbio_error_t err = pbio_tacho_get_tacho(port, &((*srv)->tacho));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Get tacho object, without additional setup.
    return pbio_dcmotor_get_dcmotor(port, &((*srv)->dcmotor));
}

static void pbio_servo_update_loop_set_state(pbio_servo_t *srv, bool update) {
    srv->run_update_loop = update;
}

bool pbio_servo_update_loop_is_running(pbio_servo_t *srv) {
    return srv->run_update_loop;
}

static pbio_error_t pbio_servo_update(pbio_servo_t *srv) {

    // Get current time
    int32_t time_now = pbdrv_clock_get_us();

    // Read the physical and estimated state
    pbio_control_state_t state;
    pbio_error_t err = pbio_servo_get_state(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Trajectory reference point
    pbio_trajectory_reference_t ref;

    // Control action to be calculated
    pbio_actuation_t actuation = PBIO_ACTUATION_COAST;
    int32_t feedback_torque = 0;
    int32_t feedforward_torque = 0;
    int32_t voltage;

    // Check if a control update is needed
    if (pbio_control_is_active(&srv->control)) {

        // Calculate feedback control signal
        pbio_control_update(&srv->control, time_now, &state, &ref, &actuation, &feedback_torque);

        // Get required feedforward torque for current reference
        feedforward_torque = pbio_observer_get_feedforward_torque(srv->observer.model, ref.rate, ref.acceleration);

        // Actuate the servo. For torque control, the torque payload is passed along. Otherwise payload is ignored.
        err = pbio_servo_actuate(srv, actuation, feedback_torque + feedforward_torque);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    // Whether or not there is control, get the ongoing actuation state so we can log it and update observer.
    bool is_coasting;
    pbio_dcmotor_get_state(srv->dcmotor, &is_coasting, &voltage);

    // Log servo state
    int32_t log_data[] = {time_now, state.count, state.rate, actuation, voltage, state.count_est, state.rate_est, feedback_torque, feedforward_torque};
    pbio_logger_update(&srv->log, log_data);

    // Update the state observer
    pbio_observer_update(&srv->observer, state.count, is_coasting, voltage);

    return PBIO_SUCCESS;
}

void pbio_servo_update_all(void) {
    pbio_error_t err;

    // Go through all motors.
    for (uint8_t i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbio_servo_t *srv = &servos[i];

        // Run update loop only if registered.
        if (srv->run_update_loop) {
            err = pbio_servo_update(srv);
            if (err != PBIO_SUCCESS) {
                // If the update failed, don't update it anymore.
                pbio_servo_update_loop_set_state(srv, false);

                // Coast the motor, letting errors pass.
                pbio_dcmotor_coast(srv->dcmotor);

                // Stop the control state.
                pbio_control_stop(&srv->control);

                // Stop higher level controls, such as drive bases.
                pbio_parent_stop(&srv->parent, false);
            }
        }
    }
}

static pbio_error_t pbio_servo_observer_reset(pbio_servo_t *srv) {

    // Get current count.
    int32_t count_now;
    pbio_error_t err = pbio_tacho_get_count(srv->tacho, &count_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Use count to initialize observer.
    pbio_observer_reset(&srv->observer, count_now, 0);
    return PBIO_SUCCESS;
}

// This function is attached to a dcmotor object, so it is able to
// stop the servo if the dcmotor needs to execute a new command.
static pbio_error_t pbio_servo_stop_from_dcmotor(void *servo, bool clear_parent) {

    // Specify pointer type.
    pbio_servo_t *srv = servo;

    // This external stop is triggered by a lower level peripheral,
    // i.e. the dc motor. So it has already has been stopped or changed state
    // electrically. All we have to do here is stop the control loop,
    // so it won't override the dcmotor to do something else.
    if (pbio_control_is_active(&srv->control)) {
        pbio_control_stop(&srv->control);

        // If we're not clearing the parent, we are done here. We don't want
        // to keep calling the drive base stop over and over.
        if (!clear_parent) {
            return PBIO_SUCCESS;
        }
    }

    // If servo control wasn't active, it's still possible that a higher
    // level abstraction is using this device. So call its stop as well.
    return pbio_parent_stop(&srv->parent, clear_parent);
}

/**
 * Sets up the servo instance to be used in an application.
 *
 * @param [in]  srv         The servo instance.
 * @param [in]  id          The I/O device type ID of the motor.
 * @param [in]  direction   The direction of positive rotation.
 * @param [in]  gear_ratio  The gear ratio of the mechanism attached to the motor.
 * @param [in]  reset_angle If true, reset the current angle to the current absolute position if supported or 0.
 * @return                  Error code.
 */
pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_iodev_type_id_t id, pbio_direction_t direction, fix16_t gear_ratio, bool reset_angle) {
    pbio_error_t err;

    // Unregister this servo from control loop updates.
    pbio_servo_update_loop_set_state(srv, false);

    // Configure tacho.
    err = pbio_tacho_setup(srv->tacho, direction, gear_ratio, reset_angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Coast and configure dcmotors, and stop its parents, if any.
    err = pbio_dcmotor_setup(srv->dcmotor, id, direction);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // This servo will be the parent of this dcmotor
    pbio_parent_set(&srv->dcmotor->parent, srv, pbio_servo_stop_from_dcmotor);

    // Reset state
    pbio_control_stop(&srv->control);

    // Load default settings for this device type
    err = pbio_servo_load_settings(&srv->control.settings, &srv->observer.model, srv->dcmotor->id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // For a servo, counts per output unit is counts per degree at the gear train output
    srv->control.settings.counts_per_unit = fix16_mul(F16C(PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE, 0), gear_ratio);

    // Initialize observer.
    err = pbio_servo_observer_reset(srv);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Now that all checks have succeeded, we know that this motor is ready.
    // So we register this servo from control loop updates.
    pbio_servo_update_loop_set_state(srv, true);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {

    pbio_error_t err;

    // Stop parent object that uses this motor, if any.
    err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If the motor was in a passive mode (coast, brake, user duty),
    // just reset angle and observer and leave physical motor state unchanged.
    if (!pbio_control_is_active(&srv->control)) {
        err = pbio_tacho_reset_angle(srv->tacho, &reset_angle, reset_to_abs);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return pbio_servo_observer_reset(srv);
    }

    // If are were busy moving, that means the reset was called while a motor
    // was running in the background. To avoid confusion as to where the motor
    // must go after the reset, we'll make it stop and hold right here right now.

    // Get the old angle
    int32_t angle_old;
    err = pbio_tacho_get_angle(srv->tacho, &angle_old);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get the old target angle that we were tracking until now
    int32_t time_ref = pbio_control_get_ref_time(&srv->control, pbdrv_clock_get_us());
    pbio_trajectory_reference_t ref;
    pbio_trajectory_get_reference(&srv->control.trajectory, time_ref, &ref);
    int32_t target_old = pbio_control_counts_to_user(&srv->control.settings, ref.count);

    // Reset the angle
    err = pbio_tacho_reset_angle(srv->tacho, &reset_angle, reset_to_abs);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reset observer to new angle
    err = pbio_servo_observer_reset(srv);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set the new target based on the old angle and the old target, after the angle reset
    int32_t new_target = reset_angle + target_old - angle_old;
    return pbio_servo_track_target(srv, new_target);
}

// Get the physical and estimated state of a single motor
pbio_error_t pbio_servo_get_state(pbio_servo_t *srv, pbio_control_state_t *state) {

    pbio_error_t err;

    // Read physical angle/counts
    err = pbio_tacho_get_count(srv->tacho, &state->count);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Read physical anglular rate
    err = pbio_tacho_get_rate(srv->tacho, &state->rate);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get estimated state
    pbio_observer_get_estimated_state(&srv->observer, &state->count_est, &state->rate_est);

    return PBIO_SUCCESS;
}

// Actuate a single motor
pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_actuation_t actuation_type, int32_t payload) {

    // Apply the calculated actuation, by type
    switch (actuation_type)
    {
        case PBIO_ACTUATION_COAST:
            return pbio_dcmotor_coast(srv->dcmotor);
        case PBIO_ACTUATION_BRAKE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, 0);
        case PBIO_ACTUATION_HOLD:
            return pbio_control_start_hold_control(&srv->control, pbdrv_clock_get_us(), payload);
        case PBIO_ACTUATION_VOLTAGE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, payload);
        case PBIO_ACTUATION_TORQUE: {
            int32_t voltage = pbio_observer_torque_to_voltage(srv->observer.model, payload);
            return pbio_dcmotor_set_voltage(srv->dcmotor, voltage);
        }
        case PBIO_ACTUATION_CONTINUE:
            return PBIO_ERROR_INVALID_OP;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get control payload
    int32_t control;
    if (after_stop == PBIO_ACTUATION_HOLD) {
        // For hold, the actuation payload is the current count
        pbio_error_t err = pbio_tacho_get_count(srv->tacho, &control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        // Otherwise the payload is zero and control stops
        control = 0;
        pbio_control_stop(&srv->control);
    }

    // Apply the actuation
    return pbio_servo_actuate(srv, after_stop, control);
}

static pbio_error_t pbio_servo_run_timed(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_control_objective_t objective, pbio_actuation_t after_stop) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get current time
    int32_t time_now = pbdrv_clock_get_us();

    // Read the physical and estimated state
    pbio_control_state_t state;
    err = pbio_servo_get_state(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver with duration converted to microseconds.
    return pbio_control_start_timed_control(&srv->control, time_now, &state, duration * US_PER_MS, target_rate, objective, after_stop);
}

pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed) {
    // Start a timed maneuver and restart it when it is done, thus running forever.
    return pbio_servo_run_timed(srv, speed, DURATION_FOREVER_MS, PBIO_CONTROL_DONE_NEVER, PBIO_ACTUATION_CONTINUE);
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop) {
    // Start a timed maneuver, duration specified by user.
    return pbio_servo_run_timed(srv, speed, duration, PBIO_CONTROL_DONE_ON_TIME, after_stop);
}

pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_actuation_t after_stop) {
    // Start a timed maneuver, and stop on stall
    return pbio_servo_run_timed(srv, speed, DURATION_FOREVER_MS, PBIO_CONTROL_DONE_ON_STALL, after_stop);
}

pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get targets in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);
    int32_t target_count = pbio_control_user_to_counts(&srv->control.settings, target);

    // Get current time
    int32_t time_now = pbdrv_clock_get_us();

    // Read the physical and estimated state
    pbio_control_state_t state;
    err = pbio_servo_get_state(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return pbio_control_start_angle_control(&srv->control, time_now, &state, target_count, target_rate, after_stop);
}

pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_actuation_t after_stop) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get targets in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);
    int32_t relative_target_count = pbio_control_user_to_counts(&srv->control.settings, angle);

    // Get current time
    int32_t time_now = pbdrv_clock_get_us();

    // Read the physical and estimated state
    pbio_control_state_t state;
    err = pbio_servo_get_state(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start the relative angle control
    return pbio_control_start_relative_angle_control(&srv->control, time_now, &state, relative_target_count, target_rate, after_stop);
}

pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    int32_t time_start = pbdrv_clock_get_us();
    int32_t target_count = pbio_control_user_to_counts(&srv->control.settings, target);

    return pbio_control_start_hold_control(&srv->control, time_start, target_count);
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
