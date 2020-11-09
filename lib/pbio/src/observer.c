// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/control.h>
#include <pbio/observer.h>

void pbio_observer_reset(pbio_observer_t *obs, int32_t count_now, int32_t rate_now) {
    obs->est_count = count_now;
    obs->est_rate = rate_now;
}

void pbio_observer_get_estimated_state(pbio_observer_t *obs, int32_t *count, int32_t *rate) {
    *count = (int32_t)obs->est_count;
    *rate = (int32_t)obs->est_rate;
}

void pbio_observer_update(pbio_observer_t *obs, int32_t count, pbio_actuation_t actuation_type, int32_t control, int32_t battery_voltage) {

    if (actuation_type != PBIO_ACTUATION_DUTY) {
        // TODO
    }

    float phi_01 = 0.00475829617417038f;
    float phi_11 = 0.904902040212507f;
    float gam_0 = 0.692925315153182f;
    float gam_1 = 272.630288453209f;
    float k_0 = 0.052359221241860474f;
    float f_low = 0.011619602790697674f;
    float obs_gain = 0.004f;

    float tau_e = (control * battery_voltage) / 10000000 * k_0;
    float tau_o = obs_gain * (count - obs->est_count);
    float tau_f = obs->est_rate > 0 ? f_low: -f_low;

    float next_count = obs->est_count + phi_01 * obs->est_rate + gam_0 * (tau_e + tau_o);
    float next_rate = phi_11 * obs->est_rate + gam_1 * (tau_e + tau_o - tau_f);

    if ((next_rate < 0) != (next_rate + gam_1 * tau_f < 0)) {
        next_rate = 0;
    }

    obs->est_count = next_count;
    obs->est_rate = next_rate;
}
