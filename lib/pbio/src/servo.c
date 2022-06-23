// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <pbdrv/clock.h>
#include <pbdrv/counter.h>
#include <pbdrv/ioport.h>

#include <pbio/angle.h>
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
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state
    pbio_control_state_t state;
    pbio_error_t err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Trajectory reference point
    pbio_trajectory_reference_t ref;

    // Control action to be calculated
    int32_t feedback_torque = 0;
    int32_t feedforward_torque = 0;

    // Check if a control update is needed
    if (pbio_control_is_active(&srv->control)) {

        // Calculate feedback control signal
        pbio_dcmotor_actuation_t requested_actuation;
        pbio_control_update(&srv->control, time_now, &state, &ref, &requested_actuation, &feedback_torque);

        // Get required feedforward torque for current reference
        feedforward_torque = pbio_observer_get_feedforward_torque(srv->observer.model, ref.speed, ref.acceleration);

        // Actuate the servo. For torque control, the torque payload is passed along. Otherwise payload is ignored.
        err = pbio_servo_actuate(srv, requested_actuation, feedback_torque + feedforward_torque);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    // Whether or not there is control, get the ongoing actuation state so we can log it and update observer.
    pbio_dcmotor_actuation_t applied_actuation;
    int32_t voltage;
    pbio_dcmotor_get_state(srv->dcmotor, &applied_actuation, &voltage);

    // Log servo state.
    int32_t log_data[] = {
        time_now,
        pbio_control_position_ctl_to_app_long(&srv->control.settings, &state.position),
        0,
        applied_actuation,
        voltage,
        pbio_control_position_ctl_to_app_long(&srv->control.settings, &state.position_estimate),
        pbio_control_position_ctl_to_app(&srv->control.settings, state.speed_estimate),
        feedback_torque,
        feedforward_torque
    };
    pbio_logger_update(&srv->log, log_data);

    // Update the state observer
    pbio_observer_update(&srv->observer, time_now, &state.position, applied_actuation, voltage);

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
                pbio_control_reset(&srv->control);

                // Stop higher level controls, such as drive bases.
                pbio_parent_stop(&srv->parent, false);
            }
        }
    }
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
        pbio_control_reset(&srv->control);

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

pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, int32_t gear_ratio, bool reset_angle) {
    pbio_error_t err;

    // Unregister this servo from control loop updates.
    pbio_servo_update_loop_set_state(srv, false);

    // Configure tacho.
    err = pbio_tacho_setup(srv->tacho, direction, reset_angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Coast and configure dcmotors, and stop its parents, if any.
    err = pbio_dcmotor_setup(srv->dcmotor, direction);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // This servo will be the parent of this dcmotor
    pbio_parent_set(&srv->dcmotor->parent, srv, pbio_servo_stop_from_dcmotor);

    // Reset state
    pbio_control_reset(&srv->control);

    // Get the device type to load relevant settings.
    pbio_iodev_type_id_t type_id;
    err = pbdrv_ioport_get_motor_device_type_id(srv->dcmotor->port, &type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Load default settings for this device type.
    err = pbio_servo_load_settings(&srv->control.settings, &srv->observer.model, type_id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // For a servo, counts per output unit is counts per degree at the gear train output
    srv->control.settings.ctl_steps_per_app_step = gear_ratio;

    // Get current angle.
    pbio_angle_t angle;
    err = pbio_tacho_get_angle(srv->tacho, &angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reset observer to current angle.
    pbio_observer_reset(&srv->observer, &angle);

    // Now that all checks have succeeded, we know that this motor is ready.
    // So we register this servo from control loop updates.
    pbio_servo_update_loop_set_state(srv, true);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {

    // If are were busy moving, that means the reset was called while a motor
    // was running in the background. To avoid confusion as to where the motor
    // must go after the reset, we'll make it stop and hold right here.
    bool hold_after_reset = pbio_control_is_active(&srv->control);

    // Get the current state so we can restore it after resetting if needed.
    pbio_dcmotor_actuation_t actuation;
    int32_t voltage;
    pbio_dcmotor_get_state(srv->dcmotor, &actuation, &voltage);

    // Stop servo and parents, if any.
    pbio_error_t err = pbio_servo_stop(srv, PBIO_CONTROL_ON_COMPLETION_COAST);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get new angle in state units.
    pbio_angle_t new_angle;
    pbio_control_position_app_to_ctl_long(&srv->control.settings, reset_angle, &new_angle);

    // Reset the tacho to the new angle.
    err = pbio_tacho_reset_angle(srv->tacho, &new_angle, reset_to_abs);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Reset observer to new angle.
    pbio_observer_reset(&srv->observer, &new_angle);

    // Restore hold if control was active during reset.
    if (hold_after_reset) {
        return pbio_servo_stop(srv, PBIO_CONTROL_ON_COMPLETION_HOLD);
    }

    // Otherwise, restore brake or passive voltage.
    if (actuation == PBIO_DCMOTOR_ACTUATION_VOLTAGE) {
        return pbio_dcmotor_set_voltage(srv->dcmotor, voltage);
    }

    // Otherwise, we were coasting, so keep doing that.
    return PBIO_SUCCESS;
}

// Get the physical and estimated state of a single motor in units of control.
// This means millidegrees at the motor output shaft, before any external gearing.
pbio_error_t pbio_servo_get_state_control(pbio_servo_t *srv, pbio_control_state_t *state) {

    pbio_error_t err;

    // Read physical angle.
    err = pbio_tacho_get_angle(srv->tacho, &state->position);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get estimated state
    pbio_observer_get_estimated_state(&srv->observer, &state->position_estimate, &state->speed_estimate);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_get_state_user(pbio_servo_t *srv, int32_t *angle, int32_t *speed) {

    // Don't allow user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get servo state.
    pbio_control_state_t state;
    pbio_error_t err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Scale by gear ratio to whole degrees.
    *angle = pbio_control_position_ctl_to_app_long(&srv->control.settings, &state.position);
    *speed = pbio_control_position_ctl_to_app(&srv->control.settings, state.speed_estimate);
    return PBIO_SUCCESS;
}

// Actuate a single motor
pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_dcmotor_actuation_t actuation_type, int32_t payload) {

    // Apply the calculated actuation, by type
    switch (actuation_type) {
        case PBIO_DCMOTOR_ACTUATION_COAST:
            return pbio_dcmotor_coast(srv->dcmotor);
        case PBIO_DCMOTOR_ACTUATION_BRAKE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, 0);
        case PBIO_DCMOTOR_ACTUATION_VOLTAGE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, payload);
        case PBIO_DCMOTOR_ACTUATION_TORQUE: {
            int32_t voltage = pbio_observer_torque_to_voltage(srv->observer.model, payload);
            return pbio_dcmotor_set_voltage(srv->dcmotor, voltage);
        }
    }

    return PBIO_ERROR_INVALID_ARG;
}

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    switch (on_completion) {
        case PBIO_CONTROL_ON_COMPLETION_COAST_SMART:
        // Same as normal coast, so fall through.
        case PBIO_CONTROL_ON_COMPLETION_COAST:
            pbio_control_stop(&srv->control);
            return pbio_servo_actuate(srv, PBIO_DCMOTOR_ACTUATION_COAST, 0);
        case PBIO_CONTROL_ON_COMPLETION_BRAKE:
            pbio_control_stop(&srv->control);
            return pbio_servo_actuate(srv, PBIO_DCMOTOR_ACTUATION_BRAKE, 0);
        case PBIO_CONTROL_ON_COMPLETION_HOLD: {
            // To hold we can just run 0 degrees from here.
            return pbio_servo_run_angle(srv, 0, 0, on_completion);
        }
        default:
            return PBIO_ERROR_INVALID_ARG;
    }
}

static pbio_error_t pbio_servo_run_timed(pbio_servo_t *srv, int32_t speed, uint32_t duration, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get current time
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state
    pbio_control_state_t state;
    err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver with duration converted to microseconds.
    return pbio_control_start_timed_control(&srv->control, time_now, &state, duration, speed, on_completion);
}

pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed) {
    // Start a timed maneuver and restart it when it is done, thus running forever.
    return pbio_servo_run_timed(srv, speed, DURATION_FOREVER_MS, PBIO_CONTROL_ON_COMPLETION_CONTINUE);
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, uint32_t duration, pbio_control_on_completion_t on_completion) {
    // Start a timed maneuver, duration specified by user.
    return pbio_servo_run_timed(srv, speed, duration, on_completion);
}

pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get current time
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state
    pbio_control_state_t state;
    err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If the speed is zero, we can't do anything meaningful, but most users
    // find it confusing if we return an error. To make sure it won't block
    // forever, we run by a relative angle to zero, which is immediately done.
    if (speed == 0) {
        return pbio_servo_run_angle(srv, speed, 0, on_completion);
    }


    return pbio_control_start_position_control(&srv->control, time_now, &state, target, speed, on_completion);
}

pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_control_on_completion_t on_completion) {

    // Don't allow new user command if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent, false);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get current time.
    uint32_t time_now = pbio_control_get_time_ticks();

    // Read the physical and estimated state.
    pbio_control_state_t state;
    err = pbio_servo_get_state_control(srv, &state);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // If the speed is zero, we can't do anything meaningful, but most users
    // find it confusing if we return an error. To make sure it won't block
    // forever, we set the angle to zero instead, so we're "done" right away.
    if (speed == 0) {
        angle = 0;
    }

    // Start the relative angle control
    return pbio_control_start_position_control_relative(&srv->control, time_now, &state, angle, speed, on_completion);
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

    // Start hold command.
    return pbio_control_start_position_control_hold(&srv->control, pbio_control_get_time_ticks(), target);
}

pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled, uint32_t *stall_duration) {

    // Don't allow access if update loop not registered.
    if (!pbio_servo_update_loop_is_running(srv)) {
        return PBIO_ERROR_INVALID_OP;
    }

    // If control is active, this provides the most accurate stall detection.
    if (pbio_control_is_active(&srv->control)) {
        *stalled = pbio_control_is_stalled(&srv->control, stall_duration);
        *stall_duration = pbio_control_time_ticks_to_ms(*stall_duration);
        return PBIO_SUCCESS;
    }

    // If we're coasting, we're not stalled.
    pbio_dcmotor_actuation_t actuation;
    int32_t voltage;
    pbio_dcmotor_get_state(srv->dcmotor, &actuation, &voltage);
    if (actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        *stall_duration = 0;
        *stalled = false;
        return PBIO_SUCCESS;
    }

    // If we're here, the user has set their own voltage or duty cycle value.
    // In this case, the best we can do is ask the observer if we're stuck.
    *stalled = pbio_observer_is_stalled(&srv->observer, pbio_control_get_time_ticks(), srv->control.settings.stall_time, stall_duration);
    *stall_duration = pbio_control_time_ticks_to_ms(*stall_duration);

    return PBIO_SUCCESS;
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
