// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef _PBIO_INTEGRATOR_H_
#define _PBIO_INTEGRATOR_H_

#include <stdint.h>

#include <pbio/config.h>

/**
 * Multiplies a value by the loop time in seconds.
 *
 * @param [in] input          Input value.
 * @return                    Input scaled by loop time.
 */
static inline bool pbio_integrator_times_loop_time(int32_t input) {
    return input / (1000 / PBIO_CONFIG_CONTROL_LOOP_TIME_MS);
}

typedef struct _pbio_speed_integrator_t {
    bool running; // Whether the integrator is running (1) or paused (0)
    uint32_t time_pause_begin; // Time at which we began pausing, stopping integration
    int32_t position_error_resumed; // Position error when integration resumed again
    int32_t speed_err_integral_paused; // Total integrated state up to previous pause.
} pbio_speed_integrator_t;

void pbio_speed_integrator_pause(pbio_speed_integrator_t *itg, uint32_t time_now, int32_t position_error);

void pbio_speed_integrator_resume(pbio_speed_integrator_t *itg, uint32_t time_now, int32_t position_error);

void pbio_speed_integrator_reset(pbio_speed_integrator_t *itg, uint32_t time_now, int32_t position_error);

int32_t pbio_speed_integrator_get_error(pbio_speed_integrator_t *itg, int32_t position_error);

bool pbio_speed_integrator_stalled(pbio_speed_integrator_t *itg, uint32_t time_now, int32_t speed_now, int32_t speed_ref, uint32_t time_stall, int32_t speed_stall);

typedef struct _pbio_position_integrator_t {
    bool trajectory_running; // Whether the trajectory is running (1) or paused (0)
    uint32_t time_pause_begin; // Time at which we began pausing most recently, stopping integration
    uint32_t time_paused_total; // Total time we spent in a paused state
    int32_t count_err_prev; // Position error in the previous control iteration
    int32_t count_err_integral; // Ongoing integral of position error
    int32_t count_err_integral_max; // Maximum value of integrator
} pbio_position_integrator_t;

uint32_t pbio_position_integrator_get_ref_time(pbio_position_integrator_t *itg, uint32_t time_now);

void pbio_position_integrator_pause(pbio_position_integrator_t *itg, uint32_t time_now);

void pbio_position_integrator_resume(pbio_position_integrator_t *itg, uint32_t time_now);

void pbio_position_integrator_reset(pbio_position_integrator_t *itg, uint32_t time_now);

int32_t pbio_position_integrator_update(pbio_position_integrator_t *itg, uint32_t time_now, int32_t position_error, int32_t position_remaining, int32_t integral_range, int32_t integral_max, int32_t integral_change_max);

bool pbio_position_integrator_stalled(pbio_position_integrator_t *itg, uint32_t time_now, int32_t speed_now, int32_t speed_ref, uint32_t time_stall, int32_t speed_stall, int32_t integral_max);

#endif // _PBIO_INTEGRATOR_H_
