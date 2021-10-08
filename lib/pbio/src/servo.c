// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <pbdrv/clock.h>
#include <pbdrv/counter.h>
#include <pbdrv/motor.h>
#include <pbdrv/battery.h>
#include <pbio/math.h>
#include <pbio/observer.h>
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

pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, fix16_t gear_ratio, bool reset_angle) {
    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    // Get and reset tacho
    err = pbio_tacho_get(srv->port, &srv->tacho, direction, gear_ratio, reset_angle);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get, coast, and configure dc motor
    err = pbio_dcmotor_get(srv->port, &srv->dcmotor, direction, true);
    if (err != PBIO_SUCCESS) {
        return err;
    }

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
    return pbio_servo_observer_reset(srv);
}

bool pbio_servo_is_connected(pbio_servo_t *srv) {
    return srv->connected;
}

void pbio_servo_set_connected(pbio_servo_t *srv, bool connected) {
    srv->connected = connected;
}

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
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
    int32_t count_ref, unused;
    pbio_trajectory_get_reference(&srv->control.trajectory, time_ref, &count_ref, &unused, &unused, &unused);
    int32_t target_old = pbio_control_counts_to_user(&srv->control.settings, count_ref);

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

// Get the physical state of a single motor
static pbio_error_t servo_get_state(pbio_servo_t *srv, int32_t *time_now, int32_t *count_now, int32_t *rate_now) {

    pbio_error_t err;

    // Read current state of this motor: current time, speed, and position
    *time_now = pbdrv_clock_get_us();
    err = pbio_tacho_get_count(srv->tacho, count_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_tacho_get_rate(srv->tacho, rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_SUCCESS;
}

// Actuate a single motor
static pbio_error_t pbio_servo_actuate(pbio_servo_t *srv, pbio_actuation_t actuation_type, int32_t control) {

    // Apply the calculated actuation, by type
    switch (actuation_type)
    {
        case PBIO_ACTUATION_COAST:
            return pbio_dcmotor_coast(srv->dcmotor);
        case PBIO_ACTUATION_BRAKE:
            return pbio_dcmotor_brake(srv->dcmotor);
        case PBIO_ACTUATION_HOLD:
            return pbio_control_start_hold_control(&srv->control, pbdrv_clock_get_us(), control);
        case PBIO_ACTUATION_VOLTAGE:
            return pbio_dcmotor_set_voltage(srv->dcmotor, control);
        case PBIO_ACTUATION_TORQUE:
            return PBIO_ERROR_NOT_IMPLEMENTED;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_update(pbio_servo_t *srv) {

    int32_t time_now;
    int32_t count_now, count_est;
    int32_t rate_now, rate_est, rate_ref;
    int32_t acceleration_ref;

    // Read the physical state
    pbio_error_t err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get estimated motor state
    pbio_observer_get_estimated_state(&srv->observer, &count_est, &rate_est);

    // Control action to be calculated
    pbio_actuation_t actuation;
    int32_t feedback_torque = 0;
    int32_t feedforward_torque = 0;
    int32_t voltage;

    // Check if a control update is needed
    if (pbio_control_is_active(&srv->control)) {

        // Calculate feedback control signal
        pbio_control_update(&srv->control, time_now, count_now, rate_now, count_est, rate_est, &actuation, &feedback_torque, &rate_ref, &acceleration_ref);

        // Get required feedforward torque
        feedforward_torque = pbio_observer_get_feedforward_torque(&srv->observer, rate_ref, acceleration_ref);

        // FIXME: There is a possible change of actuation at this point; so should pass through servo_actuate instead of actuating here

        // Convert torques to duty cycle based on model
        voltage = pbio_observer_torque_to_voltage(&srv->observer, feedback_torque + feedforward_torque);

        // Actutate the servo
        err = pbio_servo_actuate(srv, PBIO_ACTUATION_VOLTAGE, voltage);
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
    int32_t log_data[] = {0, count_now, rate_now, actuation, voltage, count_est, rate_est, feedback_torque, feedforward_torque};
    pbio_logger_update(&srv->log, log_data);

    // Update the state observer
    pbio_observer_update(&srv->observer, count_now, actuation, voltage);

    return PBIO_SUCCESS;
}

/* pbio user functions */

pbio_error_t pbio_servo_set_duty_cycle(pbio_servo_t *srv, int32_t duty_steps) {

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    pbio_control_stop(&srv->control);
    return pbio_dcmotor_set_duty_cycle_usr(srv->dcmotor, duty_steps);
}

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop) {

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
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

pbio_error_t pbio_servo_stop_force(pbio_servo_t *srv) {
    // Set control status passive so poll won't call it again
    pbio_control_stop(&srv->control);

    // Release claim from drivebases or other classes
    srv->claimed = false;

    // Try to stop / coast motor
    if (srv->dcmotor) {
        return pbio_dcmotor_coast(srv->dcmotor);
    }
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run(pbio_servo_t *srv, int32_t speed) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;

    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver, duration forever
    return pbio_control_start_timed_control(&srv->control, time_now, DURATION_FOREVER, count_now, rate_now, target_rate, srv->control.settings.abs_acceleration, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver, duration finite
    return pbio_control_start_timed_control(&srv->control, time_now, duration * US_PER_MS, count_now, rate_now, target_rate, srv->control.settings.abs_acceleration, pbio_control_on_target_time, after_stop);
}

pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver, duration forever and ending on stall
    return pbio_control_start_timed_control(&srv->control, time_now, DURATION_FOREVER, count_now, rate_now, target_rate, srv->control.settings.abs_acceleration, pbio_control_on_target_stalled, after_stop);
}

pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    // Get targets in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);
    int32_t target_count = pbio_control_user_to_counts(&srv->control.settings, target);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return pbio_control_start_angle_control(&srv->control, time_now, count_now, target_count, rate_now, target_rate, srv->control.settings.abs_acceleration, after_stop);
}

pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    // Get targets in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);
    int32_t relative_target_count = pbio_control_user_to_counts(&srv->control.settings, angle);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start the relative angle control
    return pbio_control_start_relative_angle_control(&srv->control, time_now, count_now, relative_target_count, rate_now, target_rate, srv->control.settings.abs_acceleration, after_stop);
}

pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target) {

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_BUSY;
    }

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    int32_t time_start = pbdrv_clock_get_us();
    int32_t target_count = pbio_control_user_to_counts(&srv->control.settings, target);

    return pbio_control_start_hold_control(&srv->control, time_start, target_count);
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
