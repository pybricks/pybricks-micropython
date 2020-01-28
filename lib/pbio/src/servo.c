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
#include <pbio/logger.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

#define SERVO_LOG_NUM_VALUES (12 + NUM_DEFAULT_LOG_VALUES)

// TODO: Generalize and move to config:
pbio_error_t pbio_config_get_defaults_servo(pbio_iodev_type_id_t id,
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
            *pid_kp = 200;
            *pid_ki = 100;
            *pid_kd = 0;
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

    // Get, coast, and configure dc motor
    err = pbio_dcmotor_get(srv->port, &srv->dcmotor, direction);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get default servo parameters
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

    err = pbio_config_get_defaults_servo(srv->dcmotor->id,
                                        &max_speed, &acceleration,
                                        &pid_kp, &pid_ki, &pid_kd, &tight_loop_time,
                                        &position_tolerance, &speed_tolerance, &stall_speed_limit, &stall_time);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get and reset tacho
    err = pbio_tacho_get(srv->port, &srv->tacho, direction, gear_ratio);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    // Reset state
    srv->state = PBIO_SERVO_STATE_PASSIVE;
    srv->control.is_done_func = pbio_control_always_done; // FIXME: merge state and done func

    // Set default settings for this device
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
    fix16_t counts_per_degree = F16C(PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE, 0);
    fix16_to_str(counts_per_degree, counts_per_degree_str, 3);
    fix16_to_str(fix16_div(srv->tacho->counts_per_output_unit, counts_per_degree), gear_ratio_str, 3);
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

pbio_error_t pbio_servo_reset_angle(pbio_servo_t *srv, int32_t reset_angle, bool reset_to_abs) {
    // Load motor settings and status
    pbio_error_t err;

    // Perform angle reset in case of tracking / holding
    if (srv->state == PBIO_SERVO_STATE_CONTROL_ANGLE) {
        // Get the old angle
        int32_t angle_old;
        err = pbio_tacho_get_angle(srv->tacho, &angle_old);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        // Get the old target
        int32_t target_old = pbio_math_div_i32_fix16(srv->control.trajectory.th3, srv->tacho->counts_per_output_unit);
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
    else if (srv->state == PBIO_SERVO_STATE_PASSIVE){
        return pbio_tacho_reset_angle(srv->tacho, reset_angle, reset_to_abs);
    }
    // In all other cases, stop the ongoing maneuver by coasting and then reset the angle
    else {
        err = pbio_dcmotor_coast(srv->dcmotor);
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
        srv->state = PBIO_SERVO_STATE_PASSIVE;
        break;
    case PBIO_ACTUATION_BRAKE:
        err = pbio_dcmotor_brake(srv->dcmotor);
        srv->state = PBIO_SERVO_STATE_PASSIVE;
        break;
    case PBIO_ACTUATION_HOLD:
        err = pbio_servo_track_target(srv, pbio_math_div_i32_fix16(control, srv->tacho->counts_per_output_unit));
        break;
    case PBIO_ACTUATION_DUTY:
        err = pbio_dcmotor_set_duty_cycle_sys(srv->dcmotor, control);
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
static pbio_error_t pbio_servo_log_update(pbio_servo_t *srv, int32_t time_now, int32_t count_now, int32_t rate_now, pbio_actuation_t actuation, int32_t control) {

    int32_t buf[SERVO_LOG_NUM_VALUES];


    // Get the time of reference evaluation
    int32_t time_ref = time_now;
    if (srv->state == PBIO_SERVO_STATE_CONTROL_ANGLE) {
        time_ref = pbio_count_integrator_get_ref_time(&srv->control.count_integrator, time_now);
    }

    // Log the time since start of control trajectory
    if (srv->state >= PBIO_SERVO_STATE_CONTROL_ANGLE) {
        buf[0] = (time_ref - srv->control.trajectory.t0) / 1000;
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

    // Log reference signals. These values are only meaningful for time based commands
    int32_t count_ref, count_ref_ext, rate_ref, rate_err, rate_err_integral, acceleration_ref;
    pbio_trajectory_get_reference(&srv->control.trajectory, time_ref, &count_ref, &count_ref_ext, &rate_ref, &acceleration_ref);
    pbio_rate_integrator_get_errors(&srv->control.rate_integrator, rate_now, rate_ref, count_now, count_ref, &rate_err, &rate_err_integral);
    buf[5] = count_ref;
    buf[6] = rate_err_integral;
    buf[7] = rate_ref;
    buf[8] = rate_err_integral;

    return pbio_logger_update(&srv->log, buf);
}

pbio_error_t pbio_servo_control_update(pbio_servo_t *srv) {

    // Do not service a motor claimed by a drivebase
    if (srv->state == PBIO_SERVO_STATE_CLAIMED) {
        return PBIO_SUCCESS;
    }

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
    if (srv->state == PBIO_SERVO_STATE_PASSIVE) {
        // No control, but still log state data
        pbio_passivity_t state;
        err = pbio_dcmotor_get_state(srv->dcmotor, &state, &control);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        return pbio_servo_log_update(srv, time_now, count_now, rate_now, state, control);
    }

    // Calculate controls for position based control
    if (srv->state == PBIO_SERVO_STATE_CONTROL_ANGLE) {
        control_update_angle_target(&srv->control, time_now, count_now, rate_now, &actuation, &control);
    }
    // Calculate controls for time based control
    else if (srv->state == PBIO_SERVO_STATE_CONTROL_TIMED) {
        // Get control type and signal for given state
        control_update_time_target(&srv->control, time_now, count_now, rate_now, &actuation, &control);
    }
    else {
        return PBIO_ERROR_INVALID_OP;
    }
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
    *stalled = srv->control.stalled &&
               srv->state >= PBIO_SERVO_STATE_CONTROL_ANGLE;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_set_duty_cycle(pbio_servo_t *srv, int32_t duty_steps) {
    srv->state = PBIO_SERVO_STATE_PASSIVE;
    return pbio_dcmotor_set_duty_cycle_usr(srv->dcmotor, duty_steps);
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

static pbio_error_t pbio_servo_run_time_common(pbio_servo_t *srv, int32_t speed, bool forever, int32_t duration, pbio_control_done_t stop_func, pbio_actuation_t after_stop) {

    // Get the intitial state based on physical motor state.
    int32_t time_start;
    int32_t count_now;
    int32_t rate_now;
    pbio_error_t err;

    // Get the target rate
    int32_t rate_ref = pbio_math_mul_i32_fix16(speed, srv->tacho->counts_per_output_unit);

    // Set new maneuver action and stop type, and state
    srv->control.after_stop = after_stop;
    srv->control.is_done_func = stop_func;

    // If we are continuing a timed maneuver, we can try to patch the new command onto the existing one for better continuity
    if (srv->state == PBIO_SERVO_STATE_CONTROL_TIMED) {

        // Current time
        time_start = clock_usecs();

        // Make the new trajectory and try to patch
        err = pbio_trajectory_make_time_based_patched(
            &srv->control.trajectory,
            forever,
            time_start,
            time_start + duration,
            rate_ref,
            srv->control.settings.max_rate,
            srv->control.settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    else {
        // Get the current state and time
        err = servo_get_state(srv, &time_start, &count_now, &rate_now);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // If angle based or no maneuver was ongoing, make a basic new trajectory
        // Based on the current time and current state
        err = pbio_trajectory_make_time_based(
            &srv->control.trajectory,
            forever,
            time_start,
            time_start + duration,
            count_now,
            0,
            rate_now,
            rate_ref,
            srv->control.settings.max_rate,
            srv->control.settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // New maneuver, so reset the rate integrator
        pbio_rate_integrator_reset(&srv->control.rate_integrator, time_start, count_now, count_now);

        // Set the new servo state
        srv->state = PBIO_SERVO_STATE_CONTROL_TIMED;

        // Run one control update synchronously with user command.
        err = pbio_servo_control_update(srv);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbio_servo_run(pbio_servo_t *srv, int32_t speed) {
    return pbio_servo_run_time_common(srv, speed, true, 0, pbio_control_never_done, PBIO_ACTUATION_COAST);
}

static bool run_time_is_done_func(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return time >= trajectory->t3;
}

pbio_error_t pbio_servo_run_time(pbio_servo_t *srv, int32_t speed, int32_t duration, pbio_actuation_t after_stop) {
    return pbio_servo_run_time_common(srv, speed, false, duration*US_PER_MS, run_time_is_done_func, after_stop);
}

static bool run_until_stalled_is_done_func(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    return stalled;
}

pbio_error_t pbio_servo_run_until_stalled(pbio_servo_t *srv, int32_t speed, pbio_actuation_t after_stop) {
    return pbio_servo_run_time_common(srv, speed, true, 0, run_until_stalled_is_done_func, after_stop);
}

static bool run_target_is_done_func(pbio_trajectory_t *trajectory, pbio_control_settings_t *settings, int32_t time, int32_t count, int32_t rate, bool stalled) {
    // if not enough time has expired to be done even in the ideal case, we are certainly not done
    if (time - trajectory->t3 < 0) {
        return false;
    }

    // If position is still less than the end point minus the tolerance, we are not there yet
    if (count < trajectory->th3 - settings->count_tolerance) {
        return false;
    }

    // If position is more than the end point plus the tolerance, we are too far, so not there yet
    if (count > trajectory->th3 + settings->count_tolerance) {
        return false;
    }

    // If the motor is not standing still, we are not there yet
    if (abs(rate) >= settings->rate_tolerance) {
        return false;
    }

    // There's nothing left to do, so we must be on target
    return true;
}


pbio_error_t pbio_servo_run_target(pbio_servo_t *srv, int32_t speed, int32_t target, pbio_actuation_t after_stop) {

    int32_t time_start;
    int32_t count_start;
    int32_t rate_start;
    pbio_error_t err;

    // Get the target rate and angle
    int32_t target_count = pbio_math_mul_i32_fix16(target, srv->tacho->counts_per_output_unit);
    int32_t target_rate = pbio_math_mul_i32_fix16(speed, srv->tacho->counts_per_output_unit);

    // Set new maneuver action and stop type, and state
    srv->control.after_stop = after_stop;
    srv->control.is_done_func = run_target_is_done_func;

    // If we are continuing a angle based maneuver, we can try to patch the new command onto the existing one for better continuity
    if (srv->state == PBIO_SERVO_STATE_CONTROL_ANGLE) {

        // Current time
        time_start = clock_usecs();

        // The start time must account for time spent pausing while stalled
        int32_t time_ref = pbio_count_integrator_get_ref_time(&srv->control.count_integrator, time_start);

        // Make the new trajectory and try to patch
        err = pbio_trajectory_make_angle_based_patched(
            &srv->control.trajectory,
            time_ref,
            target_count,
            target_rate,
            srv->control.settings.max_rate,
            srv->control.settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    else {

        // Get the current state and time
        err = servo_get_state(srv, &time_start, &count_start, &rate_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // If time based or no maneuver was ongoing, make a basic new trajectory
        // Based on the current time and current state
        err = pbio_trajectory_make_angle_based(
            &srv->control.trajectory,
            time_start,
            count_start,
            target_count,
            rate_start,
            target_rate,
            srv->control.settings.max_rate,
            srv->control.settings.abs_acceleration);
        if (err != PBIO_SUCCESS) {
            return err;
        }

        // New maneuver, so reset the rate integrator
        // FIXME: remove KI dependency
        int32_t integrator_max = (US_PER_SECOND/srv->control.settings.pid_ki)*srv->control.settings.max_control;
        pbio_count_integrator_reset(&srv->control.count_integrator, srv->control.trajectory.t0, srv->control.trajectory.th0, srv->control.trajectory.th0, integrator_max);

        // Set the new servo state
        srv->state = PBIO_SERVO_STATE_CONTROL_ANGLE;

        // Run one control update synchronously with user command.
        err = pbio_servo_control_update(srv);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    return PBIO_SUCCESS;
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
    pbio_error_t err;

    // Set new maneuver action and stop type
    srv->control.after_stop = PBIO_ACTUATION_COAST;
    srv->control.is_done_func = pbio_control_never_done;

    // Compute new maneuver based on user argument, starting from the initial state
    pbio_trajectory_make_stationary(&srv->control.trajectory, time_start, pbio_math_mul_i32_fix16(target, srv->tacho->counts_per_output_unit));

    // If called for the first time, set state and reset PID
    if (srv->state != PBIO_SERVO_STATE_CONTROL_ANGLE) {
        // Initialize or reset the PID control status for the given maneuver
        int32_t integrator_max = (US_PER_SECOND/srv->control.settings.pid_ki)*srv->control.settings.max_control;
        pbio_count_integrator_reset(&srv->control.count_integrator, srv->control.trajectory.t0, srv->control.trajectory.th0, srv->control.trajectory.th0, integrator_max);

        // This is an angular control maneuver
        srv->state = PBIO_SERVO_STATE_CONTROL_ANGLE;

        // Run one control update synchronously with user command
        err = pbio_servo_control_update(srv);
        if (err != PBIO_SUCCESS) {
            return err;
        }
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
