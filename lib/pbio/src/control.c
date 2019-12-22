// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>

#include <pbio/control.h>
#include <pbio/math.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>

// If the controller reach the maximum duty cycle value, this shortcut sets the stalled flag when the speed is below the stall limit.
static void stall_set_flag_if_slow(pbio_control_stalled_t *stalled,
                                   rate_t rate_now,
                                   rate_t rate_limit,
                                   ustime_t stall_time,
                                   ustime_t stall_time_limit,
                                   pbio_control_stalled_t flag) {
    if (abs(rate_now) <= rate_limit && stall_time > stall_time_limit) {
        // If the speed is less than the specified limit, set stalled flag.
        *stalled |= flag;
    }
    else {
        // Otherwise we are not yet stalled, so clear this flag.
        *stalled &= ~flag;
    }
}

// Clear the specified stall flag
static void stall_clear_flag(pbio_control_stalled_t *stalled, pbio_control_stalled_t flag) {
    *stalled &= ~flag;
}


pbio_error_t control_update_angle_target(pbio_control_t *ctl, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_actuation_t *actuation_type, int32_t *control) {
    // Trajectory and setting shortcuts for this motor
    pbio_control_status_angular_t *status = &ctl->status_angular;
    duty_t max_duty = ctl->settings.max_control;

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_ref, time_loop;
    count_t count_ref, count_err;
    rate_t rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Get the time at which we want to evaluate the reference position/velocities, for position based commands

    // In nominal operation, take the current time, minus the amount of time we have spent stalled
    if (status->ref_time_running) {
        time_ref = time_now - status->time_paused;
    }
    else {
    // When the motor stalls, we keep the time constant. This way, the position reference does
    // not continue to grow unboundedly, thus preventing a form of wind-up
        time_ref = status->time_stopped - status->time_paused;
    }

    // Get reference signals
    get_reference(time_ref, &ctl->trajectory, &count_ref, &rate_ref);

    // Position error. For position based commands, this is just the reference minus the current value
    count_err = count_ref - count_now;

    // For all commands, the speed error is simply the reference speed minus the current speed
    rate_err = rate_ref - rate_now;

    // Corresponding PD control signal
    duty_due_to_proportional = ctl->settings.pid_kp*count_err;
    duty_due_to_derivative = ctl->settings.pid_kd*rate_err;

    // Position anti-windup
    if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)) {
        // We are at the duty limit and we should prevent further position error "integration".
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(&ctl->stalled, rate_now, ctl->settings.stall_rate_limit, time_now - status->time_stopped, ctl->settings.stall_time, STALLED_PROPORTIONAL);
        // To prevent further integration, we should stop the timer if it is running
        if (status->ref_time_running) {
            // Then we must stop the time
            status->ref_time_running = false;
            // We save the time value reached now, to continue later
            status->time_stopped = time_now; // TODO: unset after clear stall?
        }
    }
    else{
        // Otherwise, the timer should be running, and we are not stalled
        stall_clear_flag(&ctl->stalled, STALLED_PROPORTIONAL);
        // So we should restart the time if it isn't already running
        if (!status->ref_time_running) {
            // Then we must restart the time
            status->ref_time_running = true;
            // We begin counting again from the stopped point
            status->time_paused += time_now - status->time_stopped;
        }
    }

    // Integrate position error
    if (status->ref_time_running) {
        time_loop = time_now - status->time_prev;
        status->err_integral += status->count_err_prev*time_loop;
    }
    status->count_err_prev = count_err;
    status->time_prev = time_now;

    // Duty cycle component due to integral position control
    duty_due_to_integral = (ctl->settings.pid_ki*(status->err_integral/US_PER_MS))/MS_PER_SECOND;

    // Integrator anti windup (stalled in the sense of integrators)
    // Limit the duty due to the integral, as well as the integral itself
    if (duty_due_to_integral > max_duty) {
        // If we are additionally also running slower than the specified stall speed limit, set status to stalled
        stall_set_flag_if_slow(&ctl->stalled, rate_now, ctl->settings.stall_rate_limit, time_now - status->time_stopped, ctl->settings.stall_time, STALLED_INTEGRAL);
        duty_due_to_integral = max_duty;
        status->err_integral = (US_PER_SECOND/ctl->settings.pid_ki)*max_duty;
    }
    else if (duty_due_to_integral < -max_duty) {
        stall_set_flag_if_slow(&ctl->stalled, rate_now, ctl->settings.stall_rate_limit, time_now - status->time_stopped, ctl->settings.stall_time, STALLED_INTEGRAL);
        duty_due_to_integral = -max_duty;
        status->err_integral = -(US_PER_SECOND/ctl->settings.pid_ki)*max_duty;
    }
    else {
        // Clear the integrator stall flag
        stall_clear_flag(&ctl->stalled, STALLED_INTEGRAL);
    }

    // Calculate duty signal
    duty = duty_due_to_proportional + duty_due_to_integral + duty_due_to_derivative;

    // Check if we are at the target and standing still, with slightly different end conditions for each mode
    if (ctl->action == RUN_TARGET &&
        // Maneuver is complete, time wise
        time_ref - ctl->trajectory.t3 >= 0 &&
        // Position is within the lower tolerated bound ...
        ctl->trajectory.th3 - ctl->settings.count_tolerance <= count_now &&
        // ... and the upper tolerated bound
        count_now <= ctl->trajectory.th3 + ctl->settings.count_tolerance &&
        // And the motor stands still.
        abs(rate_now) < ctl->settings.rate_tolerance)
    {
        // If so, we have reached our goal. So the action is the after_stop:
        *actuation_type = ctl->after_stop;

        // The payload of that action is:
        if (ctl->after_stop == PBIO_ACTUATION_HOLD) {
            // Hold at the final angle
            *control = ctl->trajectory.th3;
        }
        else {
            // no payload for coast or brake
            *control = 0;
        }
    }
    else {
        // We are not stopping, so the actuation is to apply the calculated
        // control signal
        *actuation_type = PBIO_ACTUATION_DUTY;
        *control = duty;
    }
    return PBIO_SUCCESS;
}

pbio_error_t control_update_time_target(pbio_control_t *ctl, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_actuation_t *actuation_type, int32_t *control) {

    // Trajectory and setting shortcuts for this motor
    duty_t max_duty = ctl->settings.max_control;

    // Declare time, positions, rates, and their reference value and error
    count_t count_ref, rate_err_integral;
    rate_t rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_derivative;

    // Get reference signals
    get_reference(time_now, &ctl->trajectory, &count_ref, &rate_ref);

    // For time based commands, we do not aim to drive to a specific position, but we use the
    // "proportional position control" as an exact way to implement "integral speed control".
    // The speed integral is simply the position, but the speed reference should stop integrating
    // while stalled, to prevent windup. This is built into the integrator.
    pbio_rate_integrator_get_errors(&ctl->rate_integrator, rate_now, rate_ref, count_now, count_ref, &rate_err, &rate_err_integral);

    // Corresponding PD control signal
    duty_due_to_proportional = ctl->settings.pid_kp*rate_err_integral;
    duty_due_to_derivative = ctl->settings.pid_kd*rate_err;

    // Position anti-windup
    // Check if proportional control exceeds the duty limit
    if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)) {
        pbio_rate_integrator_pause(&ctl->rate_integrator, time_now, count_now, count_ref);
    }
    else {
        pbio_rate_integrator_resume(&ctl->rate_integrator, time_now, count_now, count_ref);
    }

    // Calculate duty signal
    duty = duty_due_to_proportional + duty_due_to_derivative;

    // Check if rate controller is stalled
    bool stalled = pbio_rate_integrator_stalled(&ctl->rate_integrator, time_now, rate_now, ctl->settings.stall_time, ctl->settings.stall_rate_limit);

    // Check if objective is completed
    if ((ctl->action == RUN_TIME && time_now >= ctl->trajectory.t3) ||
        (ctl->action == RUN_STALLED && stalled)) {
        // Since we are stopping, the actuation is the after_stop action.
        *actuation_type = ctl->after_stop;

        // If that next action is holding, the corresponding signal is the
        // target position that must be held, which is the current position.
        if (*actuation_type == PBIO_ACTUATION_HOLD) {
            *control = count_now;
        }
        // Otherwise, for coasting or braking, there is no further control signal.
        else {
            *control = 0;
        }
    }
    else {
        // We are not stopping, so the actuation is to apply the calculated
        // control signal
        *actuation_type = PBIO_ACTUATION_DUTY;
        *control = duty;
    }
    return PBIO_SUCCESS;
}

void control_init_angle_target(pbio_control_t *ctl) {
    // TODO If already running, start from ref + set flag of original state

    // depending on wind up status, keep or finalize integrator state, plus maintain status

    pbio_control_status_angular_t *status = &ctl->status_angular;
    pbio_control_trajectory_t *trajectory = &ctl->trajectory;

    // For this new maneuver, we reset PID variables and related persistent control settings
    // If still running and restarting a new maneuver, however, we keep part of the PID status
    // in order to create a smooth transition from one maneuver to the next.
    // If no previous maneuver was active, just set these to zero.
    status->err_integral = 0;
    status->time_paused = 0;
    status->time_stopped = 0;
    status->time_prev = trajectory->t0;
    status->count_err_prev = 0;
    status->ref_time_running = true;
}

void control_init_time_target(pbio_control_t *ctl) {
    pbio_control_trajectory_t *trajectory = &ctl->trajectory;

    // FIXME: use correct initial time & state
    pbio_rate_integrator_reset(&ctl->rate_integrator, 0, trajectory->th0, trajectory->th0);
}

pbio_error_t pbio_control_get_limits(pbio_control_settings_t *settings,
                                     fix16_t counts_per_output_unit,
                                     int32_t *max_speed,
                                     int32_t *acceleration) {
    *max_speed = pbio_math_div_i32_fix16(settings->max_rate, counts_per_output_unit);
    *acceleration = pbio_math_div_i32_fix16(settings->abs_acceleration, counts_per_output_unit);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_control_set_limits(pbio_control_settings_t *settings,
                                     fix16_t counts_per_output_unit,
                                     int32_t max_speed,
                                     int32_t acceleration) {
    settings->max_rate = pbio_math_mul_i32_fix16(max_speed, counts_per_output_unit);
    settings->abs_acceleration = pbio_math_mul_i32_fix16(acceleration, counts_per_output_unit);
    // TODO: Add getter for max control
    return PBIO_SUCCESS;
}

pbio_error_t pbio_control_get_pid_settings(pbio_control_settings_t *settings,
                                           fix16_t counts_per_output_unit,
                                           int16_t *pid_kp,
                                           int16_t *pid_ki,
                                           int16_t *pid_kd,
                                           int32_t *tight_loop_time,
                                           int32_t *position_tolerance,
                                           int32_t *speed_tolerance,
                                           int32_t *stall_speed_limit,
                                           int32_t *stall_time) {
    // Set parameters, scaled by output scaling and gear ratio as needed
    *pid_kp = settings->pid_kp;
    *pid_ki = settings->pid_ki;
    *pid_kd = settings->pid_kd;
    *tight_loop_time = settings->tight_loop_time / US_PER_MS;
    *position_tolerance = pbio_math_div_i32_fix16(settings->count_tolerance, counts_per_output_unit);
    *speed_tolerance = pbio_math_div_i32_fix16(settings->rate_tolerance, counts_per_output_unit);
    *stall_speed_limit = pbio_math_div_i32_fix16(settings->stall_rate_limit, counts_per_output_unit);
    *stall_time = settings->stall_time / US_PER_MS;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_control_set_pid_settings(pbio_control_settings_t *settings,
                                           fix16_t counts_per_output_unit,
                                           int16_t pid_kp,
                                           int16_t pid_ki,
                                           int16_t pid_kd,
                                           int32_t tight_loop_time,
                                           int32_t position_tolerance,
                                           int32_t speed_tolerance,
                                           int32_t stall_speed_limit,
                                           int32_t stall_time) {
    // Assert that settings have positive sign
    if (pid_kp < 0 || pid_ki < 0 || pid_kd < 0 || tight_loop_time < 0 ||
        position_tolerance < 0 || speed_tolerance < 0 ||
        stall_speed_limit < 0 || stall_time < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Set parameters, scaled by output scaling and gear ratio as needed
    settings->pid_kp = pid_kp;
    settings->pid_ki = pid_ki;
    settings->pid_kd = pid_kd;
    settings->tight_loop_time = tight_loop_time * US_PER_MS;
    settings->count_tolerance = pbio_math_mul_i32_fix16(position_tolerance, counts_per_output_unit);
    settings->rate_tolerance = pbio_math_mul_i32_fix16(speed_tolerance, counts_per_output_unit);
    settings->stall_rate_limit = pbio_math_mul_i32_fix16(stall_speed_limit, counts_per_output_unit);
    settings->stall_time = stall_time * US_PER_MS;
    settings->max_control = 10000; // TODO: Add setter
    return PBIO_SUCCESS;
}
