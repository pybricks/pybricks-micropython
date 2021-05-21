// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <pbio/control.h>
#include <pbio/math.h>
#include <pbio/observer.h>

#if PBIO_CONFIG_CONTROL_MINIMAL
void pbio_observer_reset(pbio_observer_t *obs, int32_t count_now, int32_t rate_now) {
    obs->est_count = count_now * PBIO_OBSERVER_SCALE_DEG;
    obs->est_rate = rate_now * PBIO_OBSERVER_SCALE_DEG;
}

void pbio_observer_get_estimated_state(pbio_observer_t *obs, int32_t *count, int32_t *rate) {
    *count = (int32_t)(obs->est_count / PBIO_OBSERVER_SCALE_DEG);
    *rate = (int32_t)(obs->est_rate / PBIO_OBSERVER_SCALE_DEG);
}

void pbio_observer_update(pbio_observer_t *obs, int32_t count, pbio_actuation_t actuation_type, int32_t control, int32_t battery_voltage) {

    if (actuation_type != PBIO_ACTUATION_DUTY) {
        // TODO
    }

    const pbio_observer_settings_t *s = obs->settings;

    int64_t tau_e = ((int64_t)control * battery_voltage) * s->k_0 / (10000000 / PBIO_OBSERVER_SCALE_TRQ) / PBIO_OBSERVER_SCALE_HIGH;
    int64_t tau_o = s->obs_gain * ((int64_t)count * PBIO_OBSERVER_SCALE_DEG - obs->est_count) / PBIO_OBSERVER_SCALE_DEG;
    int64_t tau_f = obs->est_rate > 0 ? s->f_low: -s->f_low;

    int64_t next_count = obs->est_count + (s->phi_01 * obs->est_rate) / PBIO_OBSERVER_SCALE_HIGH + s->gam_0 * (tau_e + tau_o) * (PBIO_OBSERVER_SCALE_DEG / PBIO_OBSERVER_SCALE_LOW) / PBIO_OBSERVER_SCALE_TRQ;
    int64_t next_rate = (s->phi_11 * obs->est_rate) / PBIO_OBSERVER_SCALE_LOW + s->gam_1 * (tau_e + tau_o - tau_f) * (PBIO_OBSERVER_SCALE_DEG / PBIO_OBSERVER_SCALE_LOW) / PBIO_OBSERVER_SCALE_TRQ;

    if ((next_rate < 0) != (next_rate + s->gam_1 * tau_f * (PBIO_OBSERVER_SCALE_DEG / PBIO_OBSERVER_SCALE_LOW) / PBIO_OBSERVER_SCALE_TRQ < 0)) {
        next_rate = 0;
    }

    obs->est_count = next_count;
    obs->est_rate = next_rate;
}

int32_t pbio_observer_get_feedforward_torque(pbio_observer_t *obs, int32_t rate_ref, int32_t acceleration_ref) {
    const pbio_observer_settings_t *s = obs->settings;

    // Torque terms in micronewtons (TODO: Convert to integer math)
    int32_t friction_compensation_torque = (int32_t)(s->f_low * pbio_math_sign(rate_ref));
    int32_t back_emf_compensation_torque = (int32_t)((int64_t)s->k_0 * s->k_2 * rate_ref * (PBIO_OBSERVER_SCALE_TRQ / PBIO_OBSERVER_SCALE_HIGH) / PBIO_OBSERVER_SCALE_HIGH);
    int32_t acceleration_torque = (int32_t)((int64_t)s->k_0 * s->k_1 * acceleration_ref * (PBIO_OBSERVER_SCALE_TRQ / PBIO_OBSERVER_SCALE_HIGH) / PBIO_OBSERVER_SCALE_HIGH);

    // Scale micronewtons by battery voltage to duty (0--10000)
    return (int32_t)(friction_compensation_torque + back_emf_compensation_torque + acceleration_torque);
}


int32_t pbio_observer_torque_to_duty(pbio_observer_t *obs, int32_t desired_torque, int32_t battery_voltage) {
    return (int32_t)((int64_t)desired_torque * PBIO_OBSERVER_SCALE_HIGH * 10 / obs->settings->k_0 / battery_voltage);
}
#else
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

    const pbio_observer_settings_t *s = obs->settings;

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

int32_t pbio_observer_get_feedforward_torque(pbio_observer_t *obs, int32_t rate_ref, int32_t acceleration_ref) {
    const pbio_observer_settings_t *s = obs->settings;

    // Torque terms in micronewtons (TODO: Convert to integer math)
    int32_t friction_compensation_torque = (int32_t)(s->f_low * pbio_math_sign(rate_ref) * 1000000);
    int32_t back_emf_compensation_torque = (int32_t)(s->k_0 * s->k_2 * rate_ref * 1000000);
    int32_t acceleration_torque = (int32_t)(s->k_0 * s->k_1 * acceleration_ref * 1000000);

    // Scale micronewtons by battery voltage to duty (0--10000)
    return (int32_t)(friction_compensation_torque + back_emf_compensation_torque + acceleration_torque);
}


int32_t pbio_observer_torque_to_duty(pbio_observer_t *obs, int32_t desired_torque, int32_t battery_voltage) {
    return (int32_t)(desired_torque / obs->settings->k_0 * 10 / battery_voltage);
}
#endif // PBIO_CONFIG_CONTROL_MINIMAL
