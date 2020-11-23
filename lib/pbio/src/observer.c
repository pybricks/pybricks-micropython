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

    pbio_observer_settings_t *s = &obs->settings;

    float tau_e = (control * battery_voltage) / 10000000 * s->k_0;
    float tau_o = s->obs_gain * (count - obs->est_count);
    float tau_f = obs->est_rate > 0 ? s->f_low: -s->f_low;

    float next_count = obs->est_count + s->phi_01 * obs->est_rate + s->gam_0 * (tau_e + tau_o);
    float next_rate = s->phi_11 * obs->est_rate + s->gam_1 * (tau_e + tau_o - tau_f);

    if ((next_rate < 0) != (next_rate + s->gam_1 * tau_f < 0)) {
        next_rate = 0;
    }

    obs->est_count = next_count;
    obs->est_rate = next_rate;
}

int32_t pbio_observer_get_feed_forward(pbio_observer_t *obs, int32_t acceleration_ref, int32_t battery_voltage) {
    // TODO: express all of control update in terms of torque, then convert to duty here.
    pbio_observer_settings_t *s = &obs->settings;
    return ((s->k_1 * acceleration_ref + s->k_2 * obs->est_rate) * 10000) / battery_voltage;
}
