// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdlib.h>

#include <pbio/control.h>
#include <pbio/math.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>


pbio_error_t control_update_angle_target(pbio_control_t *ctl, ustime_t time_now, count_t count_now, rate_t rate_now, pbio_actuation_t *actuation_type, int32_t *control) {
    // Trajectory and setting shortcuts for this motor
    duty_t max_duty = ctl->settings.max_control;

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_ref;
    count_t count_ref, count_err, count_err_integral;
    rate_t rate_ref, rate_err;
    duty_t duty, duty_due_to_proportional, duty_due_to_integral, duty_due_to_derivative;

    // Get the time at which we want to evaluate the reference position/velocities, for position based commands
    // This compensates for any time we may have spent pausing when the motor was stalled.
    time_ref = pbio_count_integrator_get_ref_time(&ctl->count_integrator, time_now);

    // Get reference signals
    get_reference(time_ref, &ctl->trajectory, &count_ref, &rate_ref);

    // The speed error is the reference speed minus the current speed
    rate_err = rate_ref - rate_now;

    // Get the count error and its integral
    pbio_count_integrator_update(&ctl->count_integrator, time_now, count_now, count_ref, &count_err, &count_err_integral);

    // Corresponding PD control signal
    duty_due_to_proportional = ctl->settings.pid_kp*count_err;
    duty_due_to_derivative = ctl->settings.pid_kd*rate_err;

    // Position anti-windup
    if ((duty_due_to_proportional >= max_duty && rate_err > 0) || (duty_due_to_proportional <= -max_duty && rate_err < 0)) {
        // We are at the duty limit and we should prevent further position error "integration".
        pbio_count_integrator_pause(&ctl->count_integrator, time_now, count_now, count_ref);
    }
    else{
        // Otherwise, the timer should be running
        pbio_count_integrator_resume(&ctl->count_integrator, time_now, count_now, count_ref);
    }

    // Duty cycle component due to integral position control
    duty_due_to_integral = (ctl->settings.pid_ki*(count_err_integral/US_PER_MS))/MS_PER_SECOND;

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

    int32_t integrator_max = (US_PER_SECOND/ctl->settings.pid_ki)*ctl->settings.max_control;

    pbio_count_integrator_reset(&ctl->count_integrator, ctl->trajectory.t0, ctl->trajectory.th0, ctl->trajectory.th0, integrator_max);
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
