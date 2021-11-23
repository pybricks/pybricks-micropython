// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <pbdrv/clock.h>
#include <pbdrv/counter.h>
#include <pbdrv/motor.h>

#include <pbio/math.h>
#include <pbio/observer.h>
#include <pbio/parent.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

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
static pbio_error_t pbio_servo_stop_from_dcmotor(void *servo) {

    // Specify pointer type.
    pbio_servo_t *srv = servo;

    // This external stop is triggered by a lower level peripheral,
    // i.e. the dc motor. So it has already has been stopped or changed state
    // electrically. All we have to do here is stop the control loop,
    // so it won't override the dcmotor to do something else.
    if (pbio_control_is_active(&srv->control)) {
        pbio_control_stop(&srv->control);
        return PBIO_SUCCESS;
    }

    // If servo control wasn't active, it's still possible that a higher
    // level abstraction is using this device. So call its stop as well.
    return pbio_parent_stop(&srv->parent);
}

pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, fix16_t gear_ratio, bool reset_angle) {
    pbio_error_t err;

    // We are not initialized until setup is done.
    srv->connected = false;

    // Get and reset tacho
    err = pbio_tacho_get(srv->port, &srv->tacho, direction, gear_ratio, reset_angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get, coast (including parents), and configure dc motor
    err = pbio_dcmotor_get(srv->port, &srv->dcmotor, direction, true);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // This servo will be the parent of this dcmotor
    pbio_parent_set(&srv->dcmotor->parent, srv, pbio_servo_stop_from_dcmotor);

    // Reset state
    pbio_control_stop(&srv->control);

    // Load default settings for this device type
    err = pbio_servo_load_settings(&srv->control.settings, &srv->observer.settings, srv->dcmotor->id);
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

    // Clear parent for this device
    pbio_parent_clear(&srv->parent);

    // Now that all checks have succeeded, we know that this motor is ready.
    srv->connected = true;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {

    pbio_error_t err;

    // Stop parent object that uses this motor, if any.
    err = pbio_parent_stop(&srv->parent);
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
            return pbio_dcmotor_brake(srv->dcmotor);
        case PBIO_ACTUATION_HOLD:
            return pbio_control_start_hold_control(&srv->control, pbdrv_clock_get_us(), payload);
        case PBIO_ACTUATION_VOLTAGE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, payload);
        case PBIO_ACTUATION_TORQUE: {
            int32_t voltage = pbio_observer_torque_to_voltage(&srv->observer, payload);
            return pbio_dcmotor_set_voltage(srv->dcmotor, voltage);
        }
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_update(pbio_servo_t *srv) {

    // If the servo is not initialized (or connected), there is nothing to do.
    if (!srv->connected) {
        return PBIO_SUCCESS;
    }

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
    pbio_actuation_t actuation;
    int32_t feedback_torque = 0;
    int32_t feedforward_torque = 0;
    int32_t voltage;

    // Check if a control update is needed
    if (pbio_control_is_active(&srv->control)) {

        // Calculate feedback control signal
        pbio_control_update(&srv->control, time_now, &state, &ref, &actuation, &feedback_torque);

        // Get required feedforward torque for current reference
        feedforward_torque = pbio_observer_get_feedforward_torque(&srv->observer, ref.rate, ref.acceleration);

        // Actuate the servo. For torque control, the torque payload is passed along. Otherwise payload is ignored.
        err = pbio_servo_actuate(srv, actuation, feedback_torque + feedforward_torque);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    // Whether or not there is control, get the ongoing actuation state so we can log it and update observer.
    err = pbio_dcmotor_get_state(srv->dcmotor, (pbio_passivity_t *)&actuation, &voltage);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Log servo state
    int32_t log_data[] = {time_now, state.count, state.rate, actuation, voltage, state.count_est, state.rate_est, feedback_torque, feedforward_torque};
    pbio_logger_update(&srv->log, log_data);

    // Update the state observer
    pbio_observer_update(&srv->observer, state.count, actuation, voltage);

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop) {

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent);
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

void pbio_servo_stop_control(pbio_servo_t *srv) {

    // Set control status passive so poll won't call it again
    pbio_control_stop(&srv->control);

    // DELETEME, and drop stop too?
    if (srv->dcmotor) {
        pbio_dcmotor_stop(srv->dcmotor);
    }
}

static pbio_error_t pbio_servo_run_timed(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_control_on_target_t stop_func, pbio_actuation_t after_stop) {

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent);
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

    // Scale duration to microseconds unless it's forever.
    if (duration != DURATION_FOREVER) {
        duration *= US_PER_MS;
    }

    // Start a timed maneuver, duration finite
    return pbio_control_start_timed_control(&srv->control, time_now, &state, duration, target_rate, stop_func, after_stop);
}

pbio_error_t pbio_servo_run_forever(pbio_servo_t *srv, int32_t speed) {
    // Start a timed maneuver, duration forever
    return pbio_servo_run_timed(srv, speed, DURATION_FOREVER, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop) {
    // Start a timed maneuver, duration finite
    return pbio_servo_run_timed(srv, speed, duration, pbio_control_on_target_time, after_stop);
}

pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_actuation_t after_stop) {
    // Start a timed maneuver, duration forever and ending on stall
    return pbio_servo_run_timed(srv, speed, DURATION_FOREVER, pbio_control_on_target_stalled, after_stop);
}

pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop) {

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent);
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

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent);
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

    // Stop parent object that uses this motor, if any.
    pbio_error_t err = pbio_parent_stop(&srv->parent);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    int32_t time_start = pbdrv_clock_get_us();
    int32_t target_count = pbio_control_user_to_counts(&srv->control.settings, target);

    return pbio_control_start_hold_control(&srv->control, time_start, target_count);
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
