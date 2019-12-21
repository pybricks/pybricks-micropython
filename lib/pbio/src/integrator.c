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
    itg->time_paused = time_now;
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

bool pbio_rate_integrator_stalled(pbio_rate_integrator_t *itg, int32_t time_now, int32_t time_stall) {
    // The integrator is stalled if it has been paused more than a given amount of time
    return !itg->running && time_now - itg->time_paused > time_stall;
}
