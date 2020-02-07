// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

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

#define SERVO_LOG_NUM_VALUES (12 + NUM_DEFAULT_LOG_VALUES)

// TODO: Move to config and enable only known motors for platform
static pbio_control_settings_t settings_servo_ev3_medium = {
    .max_rate = 2000,
    .abs_acceleration = 4000,
    .rate_tolerance = 10,
    .count_tolerance = 6,
    .stall_rate_limit = 4,
    .stall_time = 200*US_PER_MS,
    .pid_kp = 400,
    .pid_ki = 600,
    .pid_kd = 5,
    .max_control = 10000,
};

static pbio_control_settings_t settings_servo_ev3_large = {
    .max_rate = 1600,
    .abs_acceleration = 3200,
    .rate_tolerance = 10,
    .count_tolerance = 6,
    .stall_rate_limit = 4,
    .stall_time = 200*US_PER_MS,
    .pid_kp = 400,
    .pid_ki = 600,
    .pid_kd = 5,
    .max_control = 10000,
};

static pbio_control_settings_t settings_servo_move_hub = {
    .max_rate = 1500,
    .abs_acceleration = 3000,
    .rate_tolerance = 5,
    .count_tolerance = 3,
    .stall_rate_limit = 2,
    .stall_time = 200*US_PER_MS,
    .pid_kp = 400,
    .pid_ki = 600,
    .pid_kd = 5,
    .max_control = 10000,
};

static pbio_control_settings_t settings_servo_default = {
    .max_rate = 1000,
    .abs_acceleration = 2000,
    .rate_tolerance = 5,
    .count_tolerance = 3,
    .stall_rate_limit = 2,
    .stall_time = 200,
    .pid_kp = 200,
    .pid_ki = 100,
    .pid_kd = 0,
    .max_control = 10000,
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
        default:
            *s = settings_servo_default;
            break;
    }
}

static pbio_servo_t servo[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, fix16_t gear_ratio) {
    pbio_error_t err;

    // Get, coast, and configure dc motor
    err = pbio_dcmotor_get(srv->port, &srv->dcmotor, direction);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get and reset tacho
    err = pbio_tacho_get(srv->port, &srv->tacho, direction, gear_ratio);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Reset state
    srv->control.type = PBIO_CONTROL_NONE;
    srv->control.is_done_func = pbio_control_always_done;

    // Load default settings for this device type
    load_servo_settings(&srv->control.settings, srv->dcmotor->id);

    // For a servo, counts per output unit is counts per degree at the gear train output
    srv->control.settings.counts_per_unit = fix16_mul(F16C(PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE, 0), gear_ratio);

    // Configure the logs for a servo
    srv->log.num_values = SERVO_LOG_NUM_VALUES;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_get(pbio_port_t port, pbio_servo_t **srv, pbio_direction_t direction, fix16_t gear_ratio) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    // Get pointer to servo object
    *srv = &servo[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    (*srv)->port = port;

    // Initialize and onfigure the servo
    pbio_error_t err = pbio_servo_setup(*srv, direction, gear_ratio);
    if (err == PBIO_SUCCESS) {
        (*srv)->connected = true;
    }
    return err;
}

pbio_error_t pbio_servo_get_ratio_settings(pbio_servo_t *srv, char *ratio_str, char *counts_per_degree_str) {
    // Compute overal gear ratio
    fix16_t gear_ratio = fix16_div(srv->control.settings.counts_per_unit, F16C(PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE, 0));
    
    // Get integer part
    int32_t ratio_int = gear_ratio >> 16;
    ratio_int = ratio_int > 999 ? 999 : ratio_int;

    // Get decimal part up to 3 digits
    int32_t ratio_dec = ((((gear_ratio << 16) >> 16))*(1000000000/fix16_one))/1000000;

    // Return as string 
    snprintf(ratio_str, 22, "%" PRId32 ".%" PRId32, ratio_int, ratio_dec);
    snprintf(counts_per_degree_str, 11, "%" PRId32, (int32_t) PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {
    // Load motor settings and status
    pbio_error_t err;

    // Perform angle reset in case of tracking / holding
    if (srv->control.type == PBIO_CONTROL_ANGLE) {
        // Get the old angle
        int32_t angle_old;
        err = pbio_tacho_get_angle(srv->tacho, &angle_old);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // Get the old target angle
        int32_t time_ref = pbio_count_integrator_get_ref_time(&srv->control.count_integrator, clock_usecs());
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
    // If the motor was in a passive mode (coast, brake, user duty), reset angle and leave state unchanged
    else if (srv->control.type == PBIO_CONTROL_NONE) {
        return pbio_tacho_reset_angle(srv->tacho, reset_angle, reset_to_abs);
    }
    // In all other cases, stop the ongoing maneuver by coasting and then reset the angle
    else {
        err = pbio_servo_stop(srv, PBIO_ACTUATION_COAST);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return pbio_tacho_reset_angle(srv->tacho, reset_angle, reset_to_abs);
    }
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

    pbio_error_t err = PBIO_SUCCESS;

    // Apply the calculated actuation, by type
    switch (actuation_type)
    {
    case PBIO_ACTUATION_COAST:
        err = pbio_dcmotor_coast(srv->dcmotor);
        break;
    case PBIO_ACTUATION_BRAKE:
        err = pbio_dcmotor_brake(srv->dcmotor);
        break;
    case PBIO_ACTUATION_HOLD:
        err = pbio_servo_track_target(srv, pbio_math_div_i32_fix16(control, srv->control.settings.counts_per_unit));
        break;
    case PBIO_ACTUATION_DUTY:
        err = pbio_dcmotor_set_duty_cycle_sys(srv->dcmotor, control);
        break;
    }

    // Handle errors during actuation
    if (err != PBIO_SUCCESS) {
        // Stop control loop
        srv->control.type = PBIO_CONTROL_NONE;

        // Attempt lowest level coast: turn off power
        pbdrv_motor_coast(srv->port);
    }
    return err;
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
        int32_t time_ref = time_now;
        // Get the time of reference evaluation
        if (srv->control.type == PBIO_CONTROL_ANGLE) {
            time_ref = pbio_count_integrator_get_ref_time(&srv->control.count_integrator, time_now);
        }
        // Log the time since start of control trajectory
        buf[0] = (time_ref - srv->control.trajectory.t0) / 1000;

        // Log reference signals. These values are only meaningful for time based commands
        int32_t count_ref, count_ref_ext, rate_ref, rate_err, rate_err_integral, acceleration_ref;
        pbio_trajectory_get_reference(&srv->control.trajectory, time_ref, &count_ref, &count_ref_ext, &rate_ref, &acceleration_ref);
        pbio_rate_integrator_get_errors(&srv->control.rate_integrator, rate_now, rate_ref, count_now, count_ref, &rate_err, &rate_err_integral);
        buf[5] = count_ref;
        buf[6] = rate_err_integral;
        buf[7] = rate_ref;
        buf[8] = rate_err_integral;
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

pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled) {
    *stalled = srv->control.type != PBIO_CONTROL_NONE && srv->control.stalled;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_set_duty_cycle(pbio_servo_t *srv, int32_t duty_steps) {
    srv->control.type = PBIO_CONTROL_NONE;
    return pbio_dcmotor_set_duty_cycle_usr(srv->dcmotor, duty_steps);
}

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop) {

    // Stop, so disable control
    srv->control.type = PBIO_CONTROL_NONE;

    // Get control payload
    int32_t control;
    if (after_stop == PBIO_ACTUATION_HOLD) {
        // For hold, the actuation payload is the current count
        pbio_error_t err = pbio_tacho_get_count(srv->tacho, &control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    else {
        // Otherwise the payload is zero
        control = 0;
    }

    // Apply the actuation
    return pbio_servo_actuate(srv, after_stop, control);
}

pbio_error_t pbio_servo_run(pbio_servo_t *srv, int32_t speed) {

    pbio_error_t err;

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver, duration forever
    return pbio_control_start_timed_control(&srv->control, time_now, DURATION_FOREVER, count_now, rate_now, target_rate, pbio_control_never_done, PBIO_ACTUATION_COAST);
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver, duration finite
    return pbio_control_start_timed_control(&srv->control, time_now, duration*US_PER_MS, count_now, rate_now, target_rate, pbio_control_time_target_done, after_stop);
}

pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Get target rate in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Start a timed maneuver, duration forever and ending on stall
    return pbio_control_start_timed_control(&srv->control, time_now, DURATION_FOREVER, count_now, rate_now, target_rate, pbio_control_until_stalled_done, after_stop);
}

pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop) {

    pbio_error_t err;

    // Get targets in unit of counts
    int32_t target_rate = pbio_control_user_to_counts(&srv->control.settings, speed);
    int32_t target_count = pbio_control_user_to_counts(&srv->control.settings, target);

    // Get the initial physical motor state.
    int32_t time_now, count_now, rate_now;
    err = servo_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    return pbio_control_start_angle_control(&srv->control, time_now, count_now, target_count, rate_now, target_rate, pbio_control_angle_target_done, after_stop);
}

pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_actuation_t after_stop) {

    // Speed  | Angle | End target  | Effect
    //  > 0   |  > 0  | now + angle | Forward
    //  > 0   |  < 0  | now + angle | Backward
    //  < 0   |  > 0  | now - angle | Backward
    //  < 0   |  < 0  | now - angle | Forward

    // Read the instantaneous angle
    int32_t angle_now;
    pbio_error_t err = pbio_tacho_get_angle(srv->tacho, &angle_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // The angle target is the instantaneous angle plus the angle to be traveled
    int32_t angle_target = angle_now + (speed < 0 ? -angle: angle);

    return pbio_servo_run_target(srv, speed, angle_target, after_stop);
}

pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target) {

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    int32_t time_start = clock_usecs();

    // Set new maneuver action and stop type
    srv->control.after_stop = PBIO_ACTUATION_COAST;
    srv->control.is_done_func = pbio_control_never_done;

    // Compute new maneuver based on user argument, starting from the initial state
    pbio_trajectory_make_stationary(&srv->control.trajectory, time_start, pbio_math_mul_i32_fix16(target, srv->control.settings.counts_per_unit));

    // If called for the first time, set state and reset PID
    if (srv->control.type != PBIO_CONTROL_ANGLE) {
        // Initialize or reset the PID control status for the given maneuver
        int32_t integrator_max = pbio_control_settings_get_max_integrator(&srv->control.settings);
        pbio_count_integrator_reset(&srv->control.count_integrator, srv->control.trajectory.t0, srv->control.trajectory.th0, srv->control.trajectory.th0, integrator_max);

        // This is an angular control maneuver
        srv->control.type = PBIO_CONTROL_ANGLE;
    }

    return PBIO_SUCCESS;
}

void _pbio_servo_reset_all(void) {
    int i;
    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbio_servo_t *srv;
        pbio_servo_get(PBDRV_CONFIG_FIRST_MOTOR_PORT + i, &srv, PBIO_DIRECTION_CLOCKWISE, 1);
    }
}

// TODO: Convert to Contiki process

// Service all the motors by calling this function at approximately constant intervals.
void _pbio_servo_poll(void) {
    int i;
    // Do the update for each motor
    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbio_servo_t *srv = &servo[i];

        // FIXME: Use a better solution skip servicing disconnected connected servos.
        if (!srv->connected) {
            continue;
        }
        srv->connected = pbio_servo_control_update(srv) == PBIO_SUCCESS;
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
