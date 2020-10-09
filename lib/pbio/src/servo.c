// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <contiki.h>

#include <pbdrv/counter.h>
#include <pbdrv/motor.h>
#include <pbio/math.h>
#include <pbio/servo.h>
#include <pbio/logger.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

#define SERVO_LOG_NUM_VALUES (9 + NUM_DEFAULT_LOG_VALUES)

// TODO: Move to config and enable only known motors for platform
static pbio_control_settings_t settings_servo_ev3_medium = {
    .max_rate = 2000,
    .abs_acceleration = 8000,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 300,
    .pid_ki = 400,
    .pid_kd = 3,
    .integral_range = 45,
    .integral_rate = 10,
    .max_control = 10000,
    .control_offset = 2000,
    .actuation_scale = 100,
};

static pbio_control_settings_t settings_servo_ev3_large = {
    .max_rate = 1600,
    .abs_acceleration = 3200,
    .rate_tolerance = 100,
    .count_tolerance = 10,
    .stall_rate_limit = 30,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 400,
    .pid_ki = 1200,
    .pid_kd = 5,
    .integral_range = 45,
    .integral_rate = 10,
    .max_control = 10000,
    .control_offset = 0,
    .actuation_scale = 100,
};

static pbio_control_settings_t settings_servo_move_hub = {
    .max_rate = 1500,
    .abs_acceleration = 5000,
    .rate_tolerance = 50,
    .count_tolerance = 6,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 400,
    .pid_ki = 600,
    .pid_kd = 5,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 10000,
    .control_offset = 2000,
    .actuation_scale = 100,
};

static pbio_control_settings_t settings_servo_boost_interactive = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 50,
    .count_tolerance = 5,
    .stall_rate_limit = 15,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 600,
    .pid_ki = 600,
    .pid_kd = 5,
    .integral_range = 45,
    .integral_rate = 3,
    .max_control = 10000,
    .control_offset = 1000,
    .actuation_scale = 100,
};

static pbio_control_settings_t settings_servo_cplus_xl = {
    .max_rate = 1000,
    .abs_acceleration = 4000,
    .rate_tolerance = 50,
    .count_tolerance = 10,
    .stall_rate_limit = 20,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 250,
    .pid_ki = 350,
    .pid_kd = 0,
    .integral_range = 45,
    .integral_rate = 5,
    .max_control = 10000,
    .control_offset = 1500,
    .actuation_scale = 100,
};

static pbio_control_settings_t settings_servo_default = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 5,
    .count_tolerance = 3,
    .stall_rate_limit = 2,
    .stall_time = 200 * US_PER_MS,
    .pid_kp = 200,
    .pid_ki = 100,
    .pid_kd = 0,
    .integral_range = 45,
    .integral_rate = 3,
    .max_control = 10000,
    .control_offset = 0,
    .actuation_scale = 100,
};

static void load_servo_settings(pbio_control_settings_t *s, pbio_iodev_type_id_t id) {
    switch (id) {
        case PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
            *s = settings_servo_ev3_medium;
            break;
        case PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR:
            *s = settings_servo_ev3_large;
            break;
        case PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR:
            *s = settings_servo_move_hub;
            break;
        case PBIO_IODEV_TYPE_ID_INTERACTIVE_MOTOR:
            *s = settings_servo_boost_interactive;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_L_MOTOR:
            *s = settings_servo_cplus_xl;
            s->control_offset = 2500;
            break;
        case PBIO_IODEV_TYPE_ID_TECHNIC_XL_MOTOR:
            *s = settings_servo_cplus_xl;
            break;
        case PBIO_IODEV_TYPE_ID_SPIKE_M_MOTOR:
            *s = settings_servo_cplus_xl;
            s->control_offset = 2500;
            break;
        case PBIO_IODEV_TYPE_ID_SPIKE_L_MOTOR:
            *s = settings_servo_cplus_xl;
            break;
        default:
            *s = settings_servo_default;
            break;
    }
}

pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, fix16_t gear_ratio) {
    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get, coast, and configure dc motor
    err = pbio_dcmotor_get(srv->port, &srv->dcmotor, direction, true);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get and reset tacho
    err = pbio_tacho_get(srv->port, &srv->tacho, direction, gear_ratio);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Reset state
    pbio_control_stop(&srv->control);

    // Load default settings for this device type
    load_servo_settings(&srv->control.settings, srv->dcmotor->id);

    // For a servo, counts per output unit is counts per degree at the gear train output
    srv->control.settings.counts_per_unit = fix16_mul(F16C(PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE, 0), gear_ratio);

    // Configure the logs for a servo
    srv->log.num_values = SERVO_LOG_NUM_VALUES;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_INVALID_OP;
    }

    // If the motor was in a passive mode (coast, brake, user duty),
    // just reset angle and leave motor state unchanged.
    if (srv->control.type == PBIO_CONTROL_NONE) {
        return pbio_tacho_reset_angle(srv->tacho, reset_angle, reset_to_abs);
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
    int32_t time_ref = pbio_control_get_ref_time(&srv->control, clock_usecs());
    int32_t count_ref, unused;
    pbio_trajectory_get_reference(&srv->control.trajectory, time_ref, &count_ref, &unused, &unused, &unused);
    int32_t target_old = pbio_control_counts_to_user(&srv->control.settings, count_ref);

    // Reset the angle
    err = pbio_tacho_reset_angle(srv->tacho, reset_angle, reset_to_abs);
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
    *time_now = clock_usecs();
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
            return pbio_control_start_hold_control(&srv->control, clock_usecs(), control);
        case PBIO_ACTUATION_DUTY:
            return pbio_dcmotor_set_duty_cycle_sys(srv->dcmotor, control);
    }

    return PBIO_SUCCESS;
}

// Log motor data for a motor that is being actively controlled
static pbio_error_t pbio_servo_log_update(pbio_servo_t *srv, int32_t time_now, int32_t count_now, int32_t rate_now, pbio_actuation_t actuation, int32_t control) {

    int32_t buf[SERVO_LOG_NUM_VALUES];
    memset(buf, 0, sizeof(buf));

    // Log the physical state of the motor
    buf[1] = count_now;
    buf[2] = rate_now;

    // Log the applied control signal
    buf[3] = actuation;
    buf[4] = control;

    // If control is active, log additional data about the maneuver
    if (srv->control.type != PBIO_CONTROL_NONE) {

        // Get the time of reference evaluation
        int32_t time_ref = pbio_control_get_ref_time(&srv->control, time_now);

        // Log the time since start of control trajectory
        buf[0] = (time_ref - srv->control.trajectory.t0) / 1000;

        // Log reference signals. These values are only meaningful for time based commands
        int32_t count_ref, count_ref_ext, rate_ref, err, err_integral, acceleration_ref;
        pbio_trajectory_get_reference(&srv->control.trajectory, time_ref, &count_ref, &count_ref_ext, &rate_ref, &acceleration_ref);

        if (srv->control.type == PBIO_CONTROL_ANGLE) {
            pbio_count_integrator_get_errors(&srv->control.count_integrator, count_now, count_ref, &err, &err_integral);
        } else {
            pbio_rate_integrator_get_errors(&srv->control.rate_integrator, rate_now, rate_ref, count_now, count_ref, &err, &err_integral);
        }

        buf[5] = count_ref;
        buf[6] = rate_ref;
        buf[7] = err; // count err for angle control, rate err for timed control
        buf[8] = err_integral;
    }

    return pbio_logger_update(&srv->log, buf);
}

pbio_error_t pbio_servo_control_update(pbio_servo_t *srv) {

    // Read the physical state
    int32_t time_now;
    int32_t count_now;
    int32_t rate_now;
    pbio_error_t err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Control action to be calculated
    pbio_actuation_t actuation;
    int32_t control;

    // Do not service a passive motor
    if (srv->control.type == PBIO_CONTROL_NONE) {
        // No control, but still log state data
        pbio_passivity_t state;
        err = pbio_dcmotor_get_state(srv->dcmotor, &state, &control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return pbio_servo_log_update(srv, time_now, count_now, rate_now, state, control);
    }

    // Calculate control signal
    control_update(&srv->control, time_now, count_now, rate_now, &actuation, &control);

    // Apply the control type and signal
    err = pbio_servo_actuate(srv, actuation, control);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Log data if logger enabled
    return pbio_servo_log_update(srv, time_now, count_now, rate_now, actuation, control);
}

/* pbio user functions */

pbio_error_t pbio_servo_set_duty_cycle(pbio_servo_t *srv, int32_t duty_steps) {

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_INVALID_OP;
    }

    pbio_control_stop(&srv->control);
    return pbio_dcmotor_set_duty_cycle_usr(srv->dcmotor, duty_steps);
}

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop) {

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_INVALID_OP;
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

    // Try to stop / coast motor whether or not initialized already
    if (srv->dcmotor) {
        return pbio_dcmotor_coast(srv->dcmotor);
    } else {
        return pbdrv_motor_coast(srv->port);
    }
}

pbio_error_t pbio_servo_run(pbio_servo_t *srv, int32_t speed) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_INVALID_OP;
    }

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;

    // FIXME: Make state getter function a control property. That way, it can
    // decide whether reading the state is needed, instead of checking control
    // status here. We do it here for now anyway to reduce I/O if the initial
    // state value is not actually used, like when control is already active.
    if (srv->control.type == PBIO_CONTROL_NONE) {
        // Get the current physical state.
        err = servo_get_state(srv, &time_now, &count_now, &rate_now);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    } else {
        time_now = clock_usecs();
    }

    // Start a timed maneuver, duration forever
    return pbio_control_start_timed_control(&srv->control, time_now, DURATION_FOREVER, count_now, rate_now, target_rate, srv->control.settings.abs_acceleration, pbio_control_on_target_never, PBIO_ACTUATION_COAST);
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Return if this servo is already in use by higher level entity
    if (srv->claimed) {
        return PBIO_ERROR_INVALID_OP;
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
        return PBIO_ERROR_INVALID_OP;
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
        return PBIO_ERROR_INVALID_OP;
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
        return PBIO_ERROR_INVALID_OP;
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
        return PBIO_ERROR_INVALID_OP;
    }

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    int32_t time_start = clock_usecs();
    int32_t target_count = pbio_control_user_to_counts(&srv->control.settings, target);

    return pbio_control_start_hold_control(&srv->control, time_start, target_count);
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
