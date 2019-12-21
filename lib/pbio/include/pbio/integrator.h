// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_INTEGRATOR_H_
#define _PBIO_INTEGRATOR_H_

#include <stdint.h>
#include <stdio.h>

typedef struct _pbio_rate_integrator_t {
    bool running; // Whether the integrator is running (1) or paused (0)
    int32_t time_paused; // Time at which we began pausing, stopping integration
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

bool pbio_rate_integrator_stalled(pbio_rate_integrator_t *itg, int32_t time_now, int32_t time_stall);

#endif // _PBIO_INTEGRATOR_H_
