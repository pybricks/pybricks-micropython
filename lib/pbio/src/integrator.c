// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <contiki.h>

#include <pbio/math.h>
#include <pbio/trajectory.h>
#include <pbio/integrator.h>

/* Rate integrator used for speed-based control */

void pbio_rate_integrator_pause(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Pause only if running
    if (!itg->running) {
        return;
    }

    // The integrator is not running anymore
    itg->running = false;

    // Increment the paused integrator state with the integrated amount between the last resume and the newly enforced pause
    itg->rate_err_integral_paused += count_ref - itg->count_ref_resumed - count + itg->count_resumed;

    // Store time at which we started pausing
    itg->time_pause_begin = time_now;
}

void pbio_rate_integrator_resume(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Resume only if paused
    if (itg->running) {
        return;
    }

    // The integrator is running again
    itg->running = true;

    // Set starting point from which we resume, if needed
    // Begin integrating again from the current point
    itg->count_ref_resumed = count_ref;
    itg->count_resumed = count;
}

void pbio_rate_integrator_reset(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Set integral to 0
    itg->rate_err_integral_paused = 0;

    // Set state to paused
    itg->running = false;

    // Resume integration
    pbio_rate_integrator_resume(itg, time_now, count, count_ref);
}

// Get reference errors and integrals
void pbio_rate_integrator_get_errors(pbio_rate_integrator_t *itg,
                                int32_t rate,
                                int32_t rate_ref,
                                int32_t count,
                                int32_t count_ref,
                                int32_t *rate_err,
                                int32_t *rate_err_integral) {

    // The rate error is simply the instantaneous error
    *rate_err = rate_ref - rate;

    // The rate error integral is at least the value at which we paused it last
    *rate_err_integral = itg->rate_err_integral_paused;

    // If integrator is active, add the exact integral since its last restart
    if (itg->running) {
        *rate_err_integral += (count_ref - itg->count_ref_resumed) - (count - itg->count_resumed);
    }
}

bool pbio_rate_integrator_stalled(pbio_rate_integrator_t *itg, int32_t time_now, int32_t rate, int32_t time_stall, int32_t rate_stall) {
    // If were running, we're not stalled
    if (itg->running) {
        return false;
    }

    // If we're still running faster than the stall limit, we're certainly not stalled.
    if (abs(rate) > rate_stall) {
        return false;
    }

    // If the integrator is paused for less than the stall time, we're still not stalled for now.
    if (time_now - itg->time_pause_begin < time_stall) {
        return false;
    }

    // All checks have failed, so we are stalled
    return true; 
}

/* Count integrator used for position-based control */

int32_t pbio_count_integrator_get_ref_time(pbio_count_integrator_t *itg, int32_t time_now) {
    // The wall time at which we are is either the current time, or whenever we stopped last
    int32_t real_time = itg->trajectory_running ? time_now : itg->time_pause_begin;

    // But we want to evaluate the reference compensating for the time we spent waiting
    return real_time - itg->time_paused_total;
}

void pbio_count_integrator_pause(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Return if already paused
    if (!itg->trajectory_running) {
        return;
    }

    // Disable the integrator
    itg->trajectory_running = false;
    itg->time_pause_begin = time_now;
}

void pbio_count_integrator_resume(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref) {

    // Return if already trajectory_running
    if (itg->trajectory_running) {
        return;
    }

    // Then we must restart the time
    itg->trajectory_running = true;

    // Increment total wait time by time elapsed since we started pausing
    itg->time_paused_total += time_now - itg->time_pause_begin;

}

void pbio_count_integrator_reset(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref, int32_t max) {

    // Reset integrator state variables
    itg->count_err_integral = 0;
    itg->time_paused_total = 0;
    itg->time_prev = time_now;
    itg->time_pause_begin = time_now;
    itg->count_err_prev = 0;
    itg->trajectory_running = false;
    itg->count_err_integral_max = max;

    // Resume integration
    pbio_count_integrator_resume(itg, time_now, count, count_ref);

}

void pbio_count_integrator_update(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref, int32_t count_target, int32_t integral_range, int32_t integral_rate) {
    // Integrate and update position error
    if (itg->trajectory_running) {

        // Counts to be to integrator: previous error times time difference
        int32_t change = itg->count_err_prev*(time_now - itg->time_prev);


        // TODO: use integral_rate setting

        // Add change if allowed to grow, or if it deflates the integral
        if (abs(count_target - count_ref) <= integral_range || abs(itg->count_err_integral + change) < abs(itg->count_err_integral)) {
            itg->count_err_integral += change;
        }

        // Limit integral to predefined bound
        if (itg->count_err_integral > itg->count_err_integral_max) {
            itg->count_err_integral = itg->count_err_integral_max;
        }
        if (itg->count_err_integral < -itg->count_err_integral_max) {
            itg->count_err_integral = -itg->count_err_integral_max;
        }
    }

    // Keep the error and time for use in next update
    itg->count_err_prev = count_ref - count;
    itg->time_prev = time_now;
}

// Get reference errors and integrals
void pbio_count_integrator_get_errors(pbio_count_integrator_t *itg, int32_t count, int32_t count_ref, int32_t *count_err, int32_t *count_err_integral) {
    // Calculate current error state
    *count_err = count_ref - count;
    *count_err_integral = itg->count_err_integral;
}

bool pbio_count_integrator_stalled(pbio_count_integrator_t *itg, int32_t time_now, int32_t rate, int32_t time_stall, int32_t rate_stall) {
    // If we're running and the integrator is not saturated, we're not stalled
    if (itg->trajectory_running && abs(itg->count_err_integral) < itg->count_err_integral_max) {
        return false;
    }

    // If we're still running faster than the stall limit, we're certainly not stalled.
    if (abs(rate) > rate_stall) {
        return false;
    }

    // If the integrator is paused for less than the stall time, we're still not stalled for now.
    if (time_now - itg->time_pause_begin < time_stall) {
        return false;
    }

    // All checks have failed, so we are stalled
    return true; 
};
