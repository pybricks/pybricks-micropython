// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2023 LEGO System A/S

/**
 * @addtogroup Integrator pbio/integrator: Integration of speed and angle signals
 *
 * Numeric and exact integration tools used by PID controllers.
 * @{
 */

#ifndef _PBIO_INTEGRATOR_H_
#define _PBIO_INTEGRATOR_H_

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/control_settings.h>

typedef struct _pbio_speed_integrator_t {
    /** Whether the integrator is running (true) or paused (false). */
    bool running;
    /** Time at which it began pausing, stopping integration */
    uint32_t time_pause_begin;
    /** Position error when integration resumed again. */
    int32_t position_error_resumed;
    /** Total integrated state up to previous pause. */
    int32_t speed_err_integral_paused;
    /** Control settings, which includes integrator settings. */
    pbio_control_settings_t *settings;
} pbio_speed_integrator_t;

// Speed integrator functions:

void pbio_speed_integrator_pause(pbio_speed_integrator_t *itg, uint32_t time_now, int32_t position_error);
void pbio_speed_integrator_resume(pbio_speed_integrator_t *itg, int32_t position_error);
void pbio_speed_integrator_reset(pbio_speed_integrator_t *itg, pbio_control_settings_t *settings);
int32_t pbio_speed_integrator_get_error(const pbio_speed_integrator_t *itg, int32_t position_error);
bool pbio_speed_integrator_stalled(const pbio_speed_integrator_t *itg, uint32_t time_now, int32_t speed_now, int32_t speed_ref);

typedef struct _pbio_position_integrator_t {
    /** Whether the trajectory is running (1) or paused (0). */
    bool trajectory_running;
    /** Time at which it began pausing most recently, stopping integration. */
    uint32_t time_pause_begin;
    /** Total time spent in a paused state. */
    uint32_t time_paused_total;
    /** Ongoing integral of position error. */
    int32_t count_err_integral;
    /** Control settings, which includes integrator settings. */
    pbio_control_settings_t *settings;
} pbio_position_integrator_t;

// Position integrator functions:

uint32_t pbio_position_integrator_get_ref_time(const pbio_position_integrator_t *itg, uint32_t time_now);
void pbio_position_integrator_pause(pbio_position_integrator_t *itg, uint32_t time_now);
bool pbio_position_integrator_is_paused(const pbio_position_integrator_t *itg);
void pbio_position_integrator_resume(pbio_position_integrator_t *itg, uint32_t time_now);
void pbio_position_integrator_reset(pbio_position_integrator_t *itg, pbio_control_settings_t *settings, uint32_t time_now);
int32_t pbio_position_integrator_update(pbio_position_integrator_t *itg, int32_t position_error, int32_t target_error);
bool pbio_position_integrator_stalled(const pbio_position_integrator_t *itg, uint32_t time_now, int32_t speed_now, int32_t speed_ref);

#endif // _PBIO_INTEGRATOR_H_

/** @} */
