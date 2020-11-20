// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_OBSERVER_H_
#define _PBIO_OBSERVER_H_

#include <stdint.h>

#include <pbio/control.h>

typedef struct _pbio_observer_settings_t {
    float phi_01;
    float phi_11;
    float gam_0;
    float gam_1;
    float k_0;
    float f_low;
    float obs_gain;
} pbio_observer_settings_t;

typedef struct _pbio_observer_t {
    float est_count;
    float est_rate;
    pbio_observer_settings_t settings;
} pbio_observer_t;

void pbio_observer_reset(pbio_observer_t *obs, int32_t count_now, int32_t rate_now);

void pbio_observer_get_estimated_state(pbio_observer_t *obs, int32_t *count, int32_t *rate);

void pbio_observer_update(pbio_observer_t *obs, int32_t count, pbio_actuation_t actuation_type, int32_t control, int32_t battery_voltage);

#endif // _PBIO_OBSERVER_H_
