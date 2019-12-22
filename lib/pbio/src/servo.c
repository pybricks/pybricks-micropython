// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>
#include <inttypes.h>

#include <contiki.h>

#include <pbdrv/counter.h>
#include <pbdrv/motor.h>
#include <pbio/math.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

// TODO: Generalize and move to config:
pbio_error_t pbio_config_get_defaults_servo(pbio_iodev_type_id_t id,
                                    fix16_t *counts_per_degree,
                                    int32_t *stall_torque_limit_pct,
                                    int32_t *duty_offset_pct,
                                    int32_t *max_speed,
                                    int32_t *acceleration,
                                    int16_t *pid_kp,
                                    int16_t *pid_ki,
                                    int16_t *pid_kd,
                                    int32_t *tight_loop_time,
                                    int32_t *position_tolerance,
                                    int32_t *speed_tolerance,
                                    int32_t *stall_speed_limit,
                                    int32_t *stall_time) {
    // Default counts per degree
    *counts_per_degree = F16C(PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE, 0);

    // Default dc motor settings
    *stall_torque_limit_pct = 100;
    *duty_offset_pct = 0;

    // Default max target run speed
    switch (id) {
        case PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
            *max_speed = 1000;
            break;
        case PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR:
            *max_speed = 800;
            break;
        case PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR:
            *max_speed = 1500;
            break;
        default:
            *max_speed = 1000;
            break;
    }

    // Default acceleration is to reach max target speed in 500 ms
    *acceleration = 2*(*max_speed);

    // Default PID settings for general purpose behavior
    switch (id) {
        case PBIO_IODEV_TYPE_ID_EV3_MEDIUM_MOTOR:
            *pid_kp = 400;
            *pid_ki = 600;
            *pid_kd = 5;
            break;
        case PBIO_IODEV_TYPE_ID_EV3_LARGE_MOTOR:
            *pid_kp = 500;
            *pid_ki = 800;
            *pid_kd = 5;
            break;
        case PBIO_IODEV_TYPE_ID_MOVE_HUB_MOTOR:
            *pid_kp = 400;
            *pid_ki = 600;
            *pid_kd = 5;
            break;
        default:
            *pid_kp = 500;
            *pid_ki = 800;
            *pid_kd = 5;
            break;
    }

    // Default tolerances for general purpose behavior
    *tight_loop_time = 100;
    *position_tolerance = 3;
    *speed_tolerance = 5;
    *stall_speed_limit = 2;
    *stall_time = 200;

    return PBIO_SUCCESS;
}

static pbio_servo_t servo[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_error_t pbio_servo_setup(pbio_servo_t *srv, pbio_direction_t direction, fix16_t gear_ratio) {
    pbio_error_t err;

    // Get device ID
    pbio_iodev_type_id_t id;
    err = pbdrv_motor_get_id(srv->port, &id);

    // If no device is found on this port, always attempt to get devices, in order to safely coast
    if (err == PBIO_ERROR_NO_DEV) {
        pbio_hbridge_get(srv->port, &srv->hbridge, direction, 0, 100);
    }

    // Return if no device is found or there was an error
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get default servo parameters
    fix16_t counts_per_degree;
    int32_t stall_torque_limit_pct;
    int32_t duty_offset_pct;
    int32_t max_speed;
    int32_t acceleration;
    int16_t pid_kp;
    int16_t pid_ki;
    int16_t pid_kd;
    int32_t tight_loop_time;
    int32_t position_tolerance;
    int32_t speed_tolerance;
    int32_t stall_speed_limit;
    int32_t stall_time;

    err = pbio_config_get_defaults_servo(id, &counts_per_degree,
                                        &stall_torque_limit_pct, &duty_offset_pct,
                                        &max_speed, &acceleration,
                                        &pid_kp, &pid_ki, &pid_kd, &tight_loop_time,
                                        &position_tolerance, &speed_tolerance, &stall_speed_limit, &stall_time);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get, coast, and configure dc motor
    err = pbio_hbridge_get(srv->port, &srv->hbridge, direction, duty_offset_pct, stall_torque_limit_pct);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get and reset tacho
    err = pbio_tacho_get(srv->port, &srv->tacho, direction, counts_per_degree, gear_ratio);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Reset state
    srv->state = PBIO_SERVO_STATE_PASSIVE;

    // Set default settings for this device
    err = pbio_hbridge_set_settings(srv->hbridge, stall_torque_limit_pct, duty_offset_pct);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_set_run_settings(srv, pbio_math_div_i32_fix16(max_speed, srv->tacho->counts_per_output_unit),
                                           pbio_math_div_i32_fix16(acceleration, srv->tacho->counts_per_output_unit));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbio_servo_set_pid_settings(srv, pid_kp, pid_ki, pid_kd, tight_loop_time, position_tolerance, speed_tolerance, stall_speed_limit, stall_time);
    if (err != PBIO_SUCCESS) {
        return err;
    }

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

pbio_error_t pbio_servo_get_gear_settings(pbio_servo_t *srv, char *gear_ratio_str, char *counts_per_degree_str) {
    fix16_to_str(srv->tacho->counts_per_degree, counts_per_degree_str, 3);
    fix16_to_str(fix16_div(srv->tacho->counts_per_output_unit, srv->tacho->counts_per_degree), gear_ratio_str, 3);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_get_run_settings(pbio_servo_t *srv, int32_t *max_speed, int32_t *acceleration) {
    return pbio_control_get_limits(&srv->control.settings, srv->tacho->counts_per_output_unit, max_speed, acceleration);
}

pbio_error_t pbio_servo_set_run_settings(pbio_servo_t *srv, int32_t max_speed, int32_t acceleration) {
    return pbio_control_set_limits(&srv->control.settings, srv->tacho->counts_per_output_unit, max_speed, acceleration);
}

pbio_error_t pbio_servo_get_pid_settings(pbio_servo_t *srv,
                                         int16_t *pid_kp,
                                         int16_t *pid_ki,
                                         int16_t *pid_kd,
                                         int32_t *tight_loop_time,
                                         int32_t *position_tolerance,
                                         int32_t *speed_tolerance,
                                         int32_t *stall_speed_limit,
                                         int32_t *stall_time) {
    return pbio_control_get_pid_settings(&srv->control.settings,
                                          srv->tacho->counts_per_output_unit,
                                          pid_kp,
                                          pid_ki,
                                          pid_kd,
                                          tight_loop_time,
                                          position_tolerance,
                                          speed_tolerance,
                                          stall_speed_limit,
                                          stall_time);
}

pbio_error_t pbio_servo_set_pid_settings(pbio_servo_t *srv,
                                         int16_t pid_kp,
                                         int16_t pid_ki,
                                         int16_t pid_kd,
                                         int32_t tight_loop_time,
                                         int32_t position_tolerance,
                                         int32_t speed_tolerance,
                                         int32_t stall_speed_limit,
                                         int32_t stall_time) {
    return pbio_control_set_pid_settings(&srv->control.settings,
                                          srv->tacho->counts_per_output_unit,
                                          pid_kp,
                                          pid_ki,
                                          pid_kd,
                                          tight_loop_time,
                                          position_tolerance,
                                          speed_tolerance,
                                          stall_speed_limit,
                                          stall_time);
}

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle) {
    // Load motor settings and status
    pbio_error_t err;

    // Perform angle reset in case of tracking / holding
    // FIXME: This does not currently capture hold after run_target
    if (srv->state == PBIO_SERVO_STATE_ANGLE_BACKGROUND && srv->control.action == TRACK_TARGET) {
        // Get the old angle
        int32_t angle_old;
        err = pbio_tacho_get_count(srv->tacho, &angle_old);
        if (err != PBIO_SUCCESS) { return err; }
        // Get the old target
        int32_t target_old = pbio_math_div_i32_fix16(srv->control.trajectory.th3, srv->tacho->counts_per_output_unit);
        // Reset the angle
        err = pbio_tacho_reset_angle(srv->tacho, reset_angle);
        if (err != PBIO_SUCCESS) { return err; }
        // Set the new target based on the old angle and the old target, after the angle reset
        int32_t new_target = reset_angle + target_old - angle_old;
        return pbio_servo_track_target(srv, new_target);
    }
    // If the motor was in a passive mode (coast, brake, user duty), reset angle and leave state unchanged
    else if (srv->state == PBIO_SERVO_STATE_PASSIVE){
        return pbio_tacho_reset_angle(srv->tacho, reset_angle);
    }
    // In all other cases, stop the ongoing maneuver by coasting and then reset the angle
    else {
        err = pbio_hbridge_coast(srv->hbridge);
        if (err != PBIO_SUCCESS) { return err; }
        return pbio_tacho_reset_angle(srv->tacho, reset_angle);
    }
}

// Get the physical state of a single motor
static pbio_error_t control_get_state(pbio_servo_t *srv, ustime_t *time_now, count_t *count_now, rate_t *rate_now) {

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
        err = pbio_hbridge_coast(srv->hbridge);
        srv->state = PBIO_SERVO_STATE_PASSIVE;
        break;
    case PBIO_ACTUATION_BRAKE:
        err = pbio_hbridge_brake(srv->hbridge);
        srv->state = PBIO_SERVO_STATE_PASSIVE;
        break;
    case PBIO_ACTUATION_HOLD:
        err = pbio_servo_track_target(srv, pbio_math_div_i32_fix16(control, srv->tacho->counts_per_output_unit));
        break;
    case PBIO_ACTUATION_DUTY:
        err = pbio_hbridge_set_duty_cycle_sys(srv->hbridge, control);
        break;
    }

    // Handle errors during actuation
    if (err != PBIO_SUCCESS) {
        // Attempt lowest level coast: turn off power
        pbdrv_motor_coast(srv->port);

        // Let foreground tasks know about error in order to stop blocking wait tasks
        srv->state = PBIO_SERVO_STATE_ERRORED;
    }
    return err;
}

// Log motor data for a motor that is being actively controlled
static pbio_error_t pbio_servo_log_update(pbio_servo_t *srv, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_actuation_t actuation, int32_t control) {

    int32_t buf[SERVO_LOG_NUM_VALUES];

    // Log the time since start of control trajectory
    if (srv->state >= PBIO_SERVO_STATE_ANGLE_BACKGROUND) {
        buf[0] = (time_now - srv->control.trajectory.t0) / 1000;
    }
    else {
        // Not applicable for passive motors
        buf[0] = 0;
    }

    // Log the physical state of the motor
    buf[1] = count_now;
    buf[2] = rate_now;

    // Log the resulting control signal
    buf[3] = actuation;
    buf[4] = control;

    return pbio_logger_update(&srv->log, buf);
}

pbio_error_t pbio_servo_control_update(pbio_servo_t *srv) {

    // Do not service a motor claimed by a drivebase
    if (srv->state == PBIO_SERVO_STATE_CLAIMED) {
        return PBIO_SUCCESS;
    }

    // Read the physical state
    ustime_t time_now;
    count_t count_now;
    rate_t rate_now;
    pbio_error_t err = control_get_state(srv, &time_now, &count_now, &rate_now);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Control action to be calculated
    pbio_actuation_t actuation;
    int32_t control;

    // Do not service a passive motor
    if (srv->state == PBIO_SERVO_STATE_PASSIVE) {
        // No control, but still log state data
        pbio_passivity_t state;
        err = pbio_hbridge_get_state(srv->hbridge, &state, &control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return pbio_servo_log_update(srv, time_now, count_now, rate_now, state, control);
    }

    // Calculate controls for position based control
    if (srv->state == PBIO_SERVO_STATE_ANGLE_BACKGROUND ||
        srv->state == PBIO_SERVO_STATE_ANGLE_FOREGROUND) {
        err = control_update_angle_target(&srv->control, time_now, count_now, rate_now, &actuation, &control);
    }
    // Calculate controls for time based control
    else if (srv->state == PBIO_SERVO_STATE_TIME_BACKGROUND ||
             srv->state == PBIO_SERVO_STATE_TIME_FOREGROUND) {
        // Get control type and signal for given state
        err = control_update_time_target(&srv->control, time_now, count_now, rate_now, &actuation, &control);
    }
    else {
        return PBIO_ERROR_INVALID_OP;
    }
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Apply the control type and signal
    err = pbio_servo_actuate(srv, actuation, control);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Log data if logger enabled
    return pbio_servo_log_update(srv, time_now, count_now, rate_now, actuation, control);
}

static pbio_error_t pbio_motor_get_initial_state(pbio_servo_t *srv, count_t *count_start, rate_t *rate_start) {

    // ustime_t time_now = clock_usecs();
    pbio_error_t err;

    // FIXME: transitions

    // if (srv->state == PBIO_SERVO_STATE_TIME_FOREGROUND || srv->state == PBIO_SERVO_STATE_TIME_BACKGROUND) {
    //     get_reference(time_now, &srv->control.trajectory, count_start, rate_start);
    // }
    // else if (srv->state == PBIO_SERVO_STATE_ANGLE_FOREGROUND || srv->state == PBIO_SERVO_STATE_ANGLE_BACKGROUND) {
    //     pbio_control_status_angular_t status = srv->control.status_angular;
    //     ustime_t time_ref = status.ref_time_running ?
    //         time_now - status.time_paused :
    //         status.time_stopped - status.time_paused;
    //     get_reference(time_ref, &srv->control.trajectory, count_start, rate_start);
    // }
    // else {
        // TODO: use generic get state functions

        // Otherwise, we are not currently in a control mode, and we start from the instantaneous motor state
        err = pbio_tacho_get_count(srv->tacho, count_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        err = pbio_tacho_get_rate(srv->tacho, rate_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    // }
    return PBIO_SUCCESS;
}

/* pbio user functions */

pbio_error_t pbio_servo_is_stalled(pbio_servo_t *srv, bool *stalled) {
    *stalled = srv->control.stalled &&
               srv->state >= PBIO_SERVO_STATE_ANGLE_BACKGROUND;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run(pbio_servo_t *srv, int32_t speed) {
    if (srv->state == PBIO_SERVO_STATE_TIME_BACKGROUND &&
        srv->control.action == RUN &&
        pbio_math_mul_i32_fix16(speed, srv->tacho->counts_per_output_unit) == srv->control.trajectory.w1) {
        // If the exact same command is already running, there is nothing we need to do
        return PBIO_SUCCESS;
    }

    // Set new maneuver action and stop type
    srv->control.action = RUN;
    srv->control.after_stop = PBIO_ACTUATION_COAST;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(srv, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based_forever(
        time_start,
        count_start,
        rate_start,
        pbio_math_mul_i32_fix16(speed, srv->tacho->counts_per_output_unit),
        srv->control.settings.max_rate,
        srv->control.settings.abs_acceleration,
        &srv->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(&srv->control);

    // Run is always in the background
    srv->state = PBIO_SERVO_STATE_TIME_BACKGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(srv);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_set_duty_cycle(pbio_servo_t *srv, int32_t duty_steps) {
    srv->state = PBIO_SERVO_STATE_PASSIVE;
    return pbio_hbridge_set_duty_cycle_usr(srv->hbridge, duty_steps);
}

pbio_error_t pbio_servo_stop(pbio_servo_t *srv, pbio_actuation_t after_stop) {

    // For most stop methods, the actuation payload is 0
    int32_t control = 0;

    // For hold, the actuation payload is the current count
    if (after_stop == PBIO_ACTUATION_HOLD) {
        pbio_error_t err = pbio_tacho_get_count(srv->tacho, &control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    // Apply the actuation
    return pbio_servo_actuate(srv, after_stop, control);
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop, bool foreground) {
    // Set new maneuver action and stop type
    srv->control.action = RUN_TIME;
    srv->control.after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(srv, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based(
        time_start,
        time_start + duration*US_PER_MS,
        count_start,
        rate_start,
        pbio_math_mul_i32_fix16(speed, srv->tacho->counts_per_output_unit),
        srv->control.settings.max_rate,
        srv->control.settings.abs_acceleration,
        &srv->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(&srv->control);

    // Set user specified foreground or background state
    srv->state = foreground ? PBIO_SERVO_STATE_TIME_FOREGROUND : PBIO_SERVO_STATE_TIME_BACKGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(srv);

    return err;
}

pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_actuation_t after_stop) {
    // Set new maneuver action and stop type
    srv->control.action = RUN_STALLED;
    srv->control.after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(srv, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_time_based_forever(
        time_start,
        count_start,
        rate_start,
        pbio_math_mul_i32_fix16(speed, srv->tacho->counts_per_output_unit),
        srv->control.settings.max_rate,
        srv->control.settings.abs_acceleration,
        &srv->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_time_target(&srv->control);

    // Run until stalled is always in the foreground
    srv->state = PBIO_SERVO_STATE_TIME_FOREGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(srv);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop, bool foreground) {
    // Set new maneuver action and stop type
    srv->control.action = RUN_TARGET;
    srv->control.after_stop = after_stop;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(srv, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    err = make_trajectory_angle_based(
        time_start,
        count_start,
        pbio_math_mul_i32_fix16(target, srv->tacho->counts_per_output_unit),
        rate_start,
        pbio_math_mul_i32_fix16(speed, srv->tacho->counts_per_output_unit),
        srv->control.settings.max_rate,
        srv->control.settings.abs_acceleration,
        &srv->control.trajectory);
    if (err != PBIO_SUCCESS) { return err; }

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(&srv->control);

    // Set user specified foreground or background state
    srv->state = foreground ? PBIO_SERVO_STATE_ANGLE_FOREGROUND : PBIO_SERVO_STATE_ANGLE_BACKGROUND;

    // Run one control update synchronously with user command.
    err = pbio_servo_control_update(srv);
    if (err != PBIO_SUCCESS) { return err; }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run_angle(pbio_servo_t *srv, int32_t speed, int32_t angle, pbio_actuation_t after_stop, bool foreground) {

    // Speed  | Angle | End target  | Effect
    //  > 0   |  > 0  | now + angle | Forward
    //  > 0   |  < 0  | now + angle | Backward
    //  < 0   |  > 0  | now - angle | Backward
    //  < 0   |  < 0  | now - angle | Forward

    // Read the instantaneous angle
    int32_t angle_now;
    pbio_error_t err = pbio_tacho_get_angle(srv->tacho, &angle_now);
    if (err != PBIO_SUCCESS) { return err; }

    // The angle target is the instantaneous angle plus the angle to be traveled
    int32_t angle_target = angle_now + (speed < 0 ? -angle: angle);

    return pbio_servo_run_target(srv, speed, angle_target, after_stop, foreground);
}


pbio_error_t pbio_servo_track_target(pbio_servo_t *srv, int32_t target) {
    // Set new maneuver action and stop type
    srv->control.action = TRACK_TARGET;
    srv->control.after_stop = PBIO_ACTUATION_COAST;

    // Get the intitial state, either based on physical motor state or ongoing maneuver
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;
    err = pbio_motor_get_initial_state(srv, &count_start, &rate_start);
    if (err != PBIO_SUCCESS) { return err; }

    // Compute new maneuver based on user argument, starting from the initial state
    make_trajectory_none(time_start, pbio_math_mul_i32_fix16(target, srv->tacho->counts_per_output_unit), 0, &srv->control.trajectory);

    // Initialize or reset the PID control status for the given maneuver
    control_init_angle_target(&srv->control);

    // Tracking a target is always a background action
    srv->state = PBIO_SERVO_STATE_ANGLE_BACKGROUND;

    // Run one control update synchronously with user command
    err = pbio_servo_control_update(srv);
    if (err != PBIO_SUCCESS) { return err; }

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
