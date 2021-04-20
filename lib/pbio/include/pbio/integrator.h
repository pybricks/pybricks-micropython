// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_INTEGRATOR_H_
#define _PBIO_INTEGRATOR_H_

#include <stdint.h>

typedef struct _pbio_rate_integrator_t {
    bool running; // Whether the integrator is running (1) or paused (0)
    int32_t time_pause_begin; // Time at which we began pausing, stopping integration
    int32_t count_resumed; // Count at which rate integration resumed again
    int32_t count_ref_resumed; // Idealized reference count at restart
    int32_t rate_err_integral_paused; // Total integrated state up to previous pause.
} pbio_rate_integrator_t;

void pbio_rate_integrator_pause(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref);

void pbio_rate_integrator_resume(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref);

void pbio_rate_integrator_reset(pbio_rate_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref);

void pbio_rate_integrator_get_errors(pbio_rate_integrator_t *itg,
    int32_t rate,
    int32_t rate_ref,
    int32_t count,
    int32_t count_ref,
    int32_t *rate_err,
    int32_t *rate_err_integral);

bool pbio_rate_integrator_stalled(pbio_rate_integrator_t *itg, int32_t time_now, int32_t rate, int32_t time_stall, int32_t rate_stall);

typedef struct _pbio_count_integrator_t {
    bool trajectory_running; // Whether the trajectory is running (1) or paused (0)
    int32_t time_pause_begin; // Time at which we began pausing most recently, stopping integration
    int32_t time_paused_total; // Total time we spent in a paused state
    int32_t count_err_prev; // Position error in the previous control iteration
    int32_t time_prev; // Time at the previous control iteratiom
    int32_t count_err_integral; // Ongoing integral of position error
    int32_t count_err_integral_max; // Maximum value of integrator
} pbio_count_integrator_t;

int32_t pbio_count_integrator_get_ref_time(pbio_count_integrator_t *itg, int32_t time_now);

void pbio_count_integrator_pause(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref);

void pbio_count_integrator_resume(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref);

void pbio_count_integrator_reset(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref, int32_t max);

void pbio_count_integrator_update(pbio_count_integrator_t *itg, int32_t time_now, int32_t count, int32_t count_ref, int32_t count_target, int32_t integral_range, int32_t integral_rate);

void pbio_count_integrator_get_errors(pbio_count_integrator_t *itg, int32_t count, int32_t count_ref, int32_t *count_err, int32_t *count_err_integral);

bool pbio_count_integrator_stalled(pbio_count_integrator_t *itg, int32_t time_now, int32_t rate, int32_t time_stall, int32_t rate_stall);

#endif // _PBIO_INTEGRATOR_H_
