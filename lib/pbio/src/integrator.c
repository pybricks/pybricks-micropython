// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/control_settings.h>
#include <pbio/integrator.h>
#include <pbio/int_math.h>

/**
 * Pauses the speed integrator at the current position error.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    time_now         The wall time (ticks).
 * @param [in]    position_error   Current position error (control units).
 */
void pbio_speed_integrator_pause(pbio_speed_integrator_t *itg, uint32_t time_now, int32_t position_error) {

    // Pause only if running
    if (!itg->running) {
        return;
    }

    // The integrator is not running anymore
    itg->running = false;

    // Increment the paused integrator state with the integrated amount between the last resume and the newly enforced pause
    itg->speed_err_integral_paused += position_error - itg->position_error_resumed;

    // Store time at which we started pausing, used only for stall flag hysteresis.
    itg->time_pause_begin = time_now;
}

/**
 * Resumes the speed integrator from the current position error.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    position_error   Current position error (control units).
 */
void pbio_speed_integrator_resume(pbio_speed_integrator_t *itg, int32_t position_error) {

    // Resume only if paused
    if (itg->running) {
        return;
    }

    // The integrator is running again
    itg->running = true;

    // Set starting point from which we resume, if needed
    // Begin integrating again from the current point
    itg->position_error_resumed = position_error;
}

/**
 * Resets the speed integrator state.
 *
 * @param [in]    itg       Speed integrator instance.
 * @param [in]    settings  Control settings instance from which to read stall settings.
 */
void pbio_speed_integrator_reset(pbio_speed_integrator_t *itg, pbio_control_settings_t *settings) {

    // Save reference to settings.
    itg->settings = settings;

    // Reset built up integral to 0.
    itg->speed_err_integral_paused = 0;

    // Set state to paused. It will resume immediately on start.
    itg->running = false;

    // Resume integration
    pbio_speed_integrator_resume(itg, 0);
}

/**
 * Gets the speed error integral accumulated thus far.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    position_error   Current position error (control units).
 * @return                         Speed error integral (position control units).
 */
int32_t pbio_speed_integrator_get_error(const pbio_speed_integrator_t *itg, int32_t position_error) {

    // The speed error integral is at least the value at which we paused it last
    int32_t speed_err_integral = itg->speed_err_integral_paused;

    // If integrator is active, add the exact integral since its last restart
    if (itg->running) {
        speed_err_integral += position_error - itg->position_error_resumed;
    }
    return speed_err_integral;
}

/**
 * Checks if the speed integrator state indicates stalling.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    time_now         The wall time (ticks).
 * @param [in]    speed_now        Current speed (control units).
 * @param [in]    speed_ref        Reference speed (control units).
 * @return                         True if stalled, false if not.
 */
bool pbio_speed_integrator_stalled(const pbio_speed_integrator_t *itg, uint32_t time_now, int32_t speed_now, int32_t speed_ref) {
    // If were running, we're not stalled
    if (itg->running) {
        return false;
    }

    // Equivalent to checking both directions, flip to positive for simplicity.
    if (speed_ref < 0) {
        speed_ref *= -1;
        speed_now *= -1;
    }

    // If we're still running faster than the stall limit, we're certainly not stalled.
    if (speed_ref != 0 && speed_now > itg->settings->stall_speed_limit) {
        return false;
    }

    // If the integrator is paused for less than the stall time, we're still not stalled for now.
    if (!pbio_control_settings_time_is_later(time_now, itg->time_pause_begin + itg->settings->stall_time)) {
        return false;
    }

    // All checks have failed, so we are stalled
    return true;
}

/**
 * Gets reference time compensated for stall duration of position controller.
 *
 * @param [in]    itg              Position integrator instance.
 * @param [in]    time_now         The wall time (ticks).
 * @return                         Wall time compensated for time spent stalling.
 */
uint32_t pbio_position_integrator_get_ref_time(const pbio_position_integrator_t *itg, uint32_t time_now) {
    // The wall time at which we are is either the current time, or whenever we stopped last.
    uint32_t real_time = itg->trajectory_running ? time_now : itg->time_pause_begin;

    // But we want to evaluate the reference compensating for the time we spent waiting.
    return real_time - itg->time_paused_total;
}

/**
 * Pauses the position integrator at the current time.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    time_now         The wall time (ticks).
 */
void pbio_position_integrator_pause(pbio_position_integrator_t *itg, uint32_t time_now) {

    // Return if already paused.
    if (!itg->trajectory_running) {
        return;
    }

    // Disable the integrator.
    itg->trajectory_running = false;
    itg->time_pause_begin = time_now;
}

/**
 * Tests if the position integrator is paused.
 *
 * @param [in]    itg              Speed integrator instance.
 * @return                         True if integration is paused, false if not.
 */
bool pbio_position_integrator_is_paused(const pbio_position_integrator_t *itg) {
    return !itg->trajectory_running;
}

/**
 * Resumes the position integrator at the current time.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    time_now         The wall time (ticks).
 */
void pbio_position_integrator_resume(pbio_position_integrator_t *itg, uint32_t time_now) {

    // Return if already trajectory_running
    if (itg->trajectory_running) {
        return;
    }

    // Then we must restart the time
    itg->trajectory_running = true;

    // Increment total wait time by time elapsed since we started pausing
    itg->time_paused_total += time_now - itg->time_pause_begin;
}

/**
 * Resets the position integrator state.
 *
 * @param [in]    itg       Speed integrator instance.
 * @param [in]    settings  Control settings instance from which to read stall settings.
 * @param [in]    time_now  The wall time (ticks).
 */
void pbio_position_integrator_reset(pbio_position_integrator_t *itg, pbio_control_settings_t *settings, uint32_t time_now) {

    // Save reference to settings.
    itg->settings = settings;

    // Reset integrator state variables
    itg->count_err_integral = 0;
    itg->time_paused_total = 0;
    itg->time_pause_begin = time_now;
    itg->trajectory_running = false;

    // Resume integration
    pbio_position_integrator_resume(itg, time_now);

}

/**
 * Updates the position integrator state with the latest error.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    position_error   Current position error (position control units).
 * @param [in]    target_error     Remaining error to the endpoint (position control units).
 * @return                         Integrator state value (position control units).
 */
int32_t pbio_position_integrator_update(pbio_position_integrator_t *itg, int32_t position_error, int32_t target_error) {

    int32_t error_now = position_error;

    // Check if integrator magnitude would decrease due to this error
    bool decrease = pbio_int_math_abs(itg->count_err_integral + pbio_control_settings_mul_by_loop_time(error_now)) < pbio_int_math_abs(itg->count_err_integral);

    // Integrate and update position error
    if (itg->trajectory_running || decrease) {

        // If not deceasing, so growing, limit error growth by maximum integral rate
        if (!decrease) {
            error_now = error_now > itg->settings->integral_change_max ? itg->settings->integral_change_max : error_now;
            error_now = error_now < -itg->settings->integral_change_max ? -itg->settings->integral_change_max : error_now;

            // It might be decreasing now after all (due to integral sign change), so re-evaluate
            decrease = pbio_int_math_abs(itg->count_err_integral + pbio_control_settings_mul_by_loop_time(error_now)) < pbio_int_math_abs(itg->count_err_integral);
        }

        // Specify in which region integral control should be active. This is
        // at least the error that would still lead to maximum  proportional
        // control, with a factor of 2 so we begin integrating a bit sooner.
        int32_t integral_range_upper = pbio_control_settings_div_by_gain(itg->settings->actuation_max, itg->settings->pid_kp) * 2;

        // Add change if we are near (but not too near) target, or always if it decreases the integral magnitude.
        if ((pbio_int_math_abs(target_error) >= itg->settings->integral_deadzone &&
             pbio_int_math_abs(target_error) <= integral_range_upper) || decrease) {
            itg->count_err_integral += pbio_control_settings_mul_by_loop_time(error_now);
        }

        // Limit integral to value that leads to maximum actuation, i.e. max actuation / ki.
        itg->count_err_integral = pbio_int_math_clamp(itg->count_err_integral,
            pbio_control_settings_div_by_gain(itg->settings->actuation_max, itg->settings->pid_ki));
    }

    // Return current value.
    return itg->count_err_integral;
}

/**
 * Checks if the position integrator state indicates stalling.
 *
 * @param [in]    itg              Speed integrator instance.
 * @param [in]    time_now         The wall time (ticks).
 * @param [in]    speed_now        Current speed (control units).
 * @param [in]    speed_ref        Reference speed (control units).
 * @return                         True if stalled, false if not.
 */
bool pbio_position_integrator_stalled(const pbio_position_integrator_t *itg, uint32_t time_now, int32_t speed_now, int32_t speed_ref) {

    // Get integral value that would lead to maximum actuation.
    int32_t integral_max = pbio_control_settings_div_by_gain(itg->settings->actuation_max, itg->settings->pid_ki);

    // Whether the integrator is saturated.
    bool saturated = integral_max != 0 && pbio_int_math_abs(itg->count_err_integral) >= integral_max;

    // If we're running and the integrator is not saturated, we're not stalled.
    if (itg->trajectory_running && !saturated) {
        return false;
    }

    // Equivalent to checking both directions, flip to positive for simplicity.
    if (speed_ref < 0) {
        speed_ref *= -1;
        speed_now *= -1;
    }

    // If we're still running faster than the stall limit, we're certainly not stalled.
    if (speed_ref != 0 && speed_now > itg->settings->stall_speed_limit) {
        return false;
    }

    // If the integrator is paused for less than the stall time, we're still not stalled for now.
    if (!pbio_control_settings_time_is_later(time_now, itg->time_pause_begin + itg->settings->stall_time)) {
        return false;
    }

    // All checks have failed, so we are stalled
    return true;
};
