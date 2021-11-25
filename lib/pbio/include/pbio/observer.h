// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_OBSERVER_H_
#define _PBIO_OBSERVER_H_

#include <stdint.h>

#include <pbio/control.h>

#if PBIO_CONFIG_CONTROL_MINIMAL
#define PBIO_OBSERVER_SCALE_DEG (10000)
#define PBIO_OBSERVER_SCALE_TRQ (1000000)
#define PBIO_OBSERVER_SCALE_LOW (1000)
#define PBIO_OBSERVER_SCALE_HIGH (1000000)
#else
#define PBIO_OBSERVER_SCALE_DEG (1.0f)
#define PBIO_OBSERVER_SCALE_TRQ (1.0f)
#define PBIO_OBSERVER_SCALE_LOW (1.0f)
#define PBIO_OBSERVER_SCALE_HIGH (1.0f)
#endif

// Observer gains in micronewtonmeters per degree of error.
// Medium and high gains must be integer multiples of the
// lowest gain value.
#define PBIO_OBSERVER_GAINS(LOW, MEDIUM, HIGH) \
    (((LOW) << 16 | (MEDIUM / LOW) << 8 | (HIGH / LOW)))

typedef struct _pbio_observer_settings_t {
    #if PBIO_CONFIG_CONTROL_MINIMAL
    int32_t phi_01;
    int32_t phi_11;
    int32_t gam_0;
    int32_t gam_1;
    int32_t k_0;
    int32_t k_1;
    int32_t k_2;
    int32_t f_low;
    int32_t obs_gain;
    #else
    float phi_01;
    float phi_11;
    float gam_0;
    float gam_1;
    float k_0;
    float k_1;
    float k_2;
    float f_low;
    #endif
    uint32_t obs_gains;
} pbio_observer_settings_t;

typedef struct _pbio_observer_t {
    #if PBIO_CONFIG_CONTROL_MINIMAL
    int64_t est_count;
    int64_t est_rate;
    #else
    float est_count;
    float est_rate;
    #endif
    const pbio_observer_settings_t *settings;
} pbio_observer_t;

void pbio_observer_reset(pbio_observer_t *obs, int32_t count_now, int32_t rate_now);

void pbio_observer_get_estimated_state(pbio_observer_t *obs, int32_t *count, int32_t *rate);

void pbio_observer_update(pbio_observer_t *obs, int32_t count, bool is_coasting, int32_t voltage);

int32_t pbio_observer_get_feedforward_torque(pbio_observer_t *obs, int32_t rate_ref, int32_t acceleration_ref);

int32_t pbio_observer_torque_to_voltage(pbio_observer_t *obs, int32_t desired_torque);

#endif // _PBIO_OBSERVER_H_
