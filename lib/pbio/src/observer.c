// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

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

void pbio_observer_update(pbio_observer_t *obs, int32_t count, bool is_coasting, int32_t voltage) {

    if (is_coasting) {
        // TODO
    }

    const pbio_observer_model_t *m = obs->model;

    // Torque due to duty cycle
    int64_t tau_e = (int64_t)voltage * m->k_0 / 1000 * (PBIO_OBSERVER_SCALE_TRQ / PBIO_OBSERVER_SCALE_HIGH);

    // Friction torque
    int64_t tau_f = obs->est_rate > 0 ? m->f_low: -m->f_low;

    // Unpack observer gain constants.
    int64_t k_low = m->obs_gains >> 16;
    int64_t k_med = k_low * ((m->obs_gains & 0x0000FF00) >> 8);
    int64_t k_high = k_low * (m->obs_gains & 0x000000FF);

    // Below this error, the virtual spring stiffness is low.
    int64_t r1 = 5 * PBIO_OBSERVER_SCALE_DEG;

    // Below this error, the virtual spring stiffness is medium, above is high.
    int64_t r2 = 25 * PBIO_OBSERVER_SCALE_DEG;

    // Get estimation error
    int64_t est_err = (int64_t)count * PBIO_OBSERVER_SCALE_DEG - obs->est_count;

    // Compute torque for estimation error correction as a piecewise affine spring
    int64_t tau_o = 0;
    if (abs(est_err) < r1) {
        tau_o = k_low * est_err / PBIO_OBSERVER_SCALE_DEG;
    } else if (abs(est_err) < r2) {
        tau_o = (k_low * r1 + (abs(est_err) - r1) * k_med) * pbio_math_sign(est_err) / PBIO_OBSERVER_SCALE_DEG;
    } else {
        tau_o = (k_low * r1 + k_med * (r2 - r1) + (abs(est_err) - r2) * k_high) * pbio_math_sign(est_err) / PBIO_OBSERVER_SCALE_DEG;
    }

    int64_t next_count = obs->est_count + (m->phi_01 * obs->est_rate) / PBIO_OBSERVER_SCALE_HIGH + m->gam_0 * (tau_e + tau_o) * (PBIO_OBSERVER_SCALE_DEG / PBIO_OBSERVER_SCALE_LOW) / PBIO_OBSERVER_SCALE_TRQ;
    int64_t next_rate = (m->phi_11 * obs->est_rate) / PBIO_OBSERVER_SCALE_LOW + m->gam_1 * (tau_e + tau_o - tau_f) * (PBIO_OBSERVER_SCALE_DEG / PBIO_OBSERVER_SCALE_LOW) / PBIO_OBSERVER_SCALE_TRQ;

    if ((next_rate < 0) != (next_rate + m->gam_1 * tau_f * (PBIO_OBSERVER_SCALE_DEG / PBIO_OBSERVER_SCALE_LOW) / PBIO_OBSERVER_SCALE_TRQ < 0)) {
        next_rate = 0;
    }

    obs->est_count = next_count;
    obs->est_rate = next_rate;
}

int32_t pbio_observer_get_feedforward_torque(const pbio_observer_model_t *model, int32_t rate_ref, int32_t acceleration_ref) {
    // Torque terms in micronewtons
    int32_t friction_compensation_torque = (int32_t)(model->f_low * pbio_math_sign(rate_ref));
    int32_t back_emf_compensation_torque = (int32_t)((int64_t)model->k_0 * model->k_2 * rate_ref * (PBIO_OBSERVER_SCALE_TRQ / PBIO_OBSERVER_SCALE_HIGH) / PBIO_OBSERVER_SCALE_HIGH);
    int32_t acceleration_torque = (int32_t)((int64_t)model->k_0 * model->k_1 * acceleration_ref * (PBIO_OBSERVER_SCALE_TRQ / PBIO_OBSERVER_SCALE_HIGH) / PBIO_OBSERVER_SCALE_HIGH);

    // Scale micronewtons by battery voltage to duty (0 to PBDRV_MOTOR_DRIVER_MAX_DUTY)
    return (int32_t)(friction_compensation_torque + back_emf_compensation_torque + acceleration_torque);
}

int32_t pbio_observer_torque_to_voltage(const pbio_observer_model_t *model, int32_t desired_torque) {
    return (int32_t)((int64_t)desired_torque * (PBIO_OBSERVER_SCALE_HIGH / 1000) / model->k_0);
}

int32_t pbio_observer_voltage_to_torque(const pbio_observer_model_t *model, int32_t voltage) {
    return (int32_t)((int64_t)model->k_0 * (PBIO_OBSERVER_SCALE_TRQ / PBIO_OBSERVER_SCALE_HIGH) * voltage / 1000);
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

void pbio_observer_update(pbio_observer_t *obs, int32_t count, bool is_coasting, int32_t voltage) {

    if (is_coasting) {
        // TODO
    }

    const pbio_observer_model_t *m = obs->model;

    // Torque due to voltage
    float tau_e = voltage / 1000 * m->k_0;

    // Friction torque
    float tau_f = obs->est_rate > 0 ? m->f_low: -m->f_low;

    // Unpack observer gain constants.
    float k_low = ((float)(m->obs_gains >> 16)) / 1000000;
    float k_med = k_low * ((m->obs_gains & 0x0000FF00) >> 8);
    float k_high = k_low * (m->obs_gains & 0x000000FF);

    // Below this error, the virtual spring stiffness is low.
    float r1 = 5;

    // Below this error, the virtual spring stiffness is medium, above is high.
    float r2 = 25;

    // Get estimation error
    float est_err = count - obs->est_count;

    // Compute torque for estimation error correction as a piecewise affine spring
    float tau_o = 0;
    if (fabsf(est_err) < r1) {
        tau_o = k_low * est_err;
    } else if (fabsf(est_err) < r2) {
        tau_o = copysignf(k_low * r1 + (fabsf(est_err) - r1) * k_med, est_err);
    } else {
        tau_o = copysignf(k_low * r1 + k_med * (r2 - r1) + (fabsf(est_err) - r2) * k_high, est_err);
    }

    // Get next state given total torque
    float next_count = obs->est_count + m->phi_01 * obs->est_rate + m->gam_0 * (tau_e + tau_o);
    float next_rate = m->phi_11 * obs->est_rate + m->gam_1 * (tau_e + tau_o - tau_f);

    if ((next_rate < 0) != (next_rate + m->gam_1 * tau_f < 0)) {
        next_rate = 0;
    }

    obs->est_count = next_count;
    obs->est_rate = next_rate;
}

int32_t pbio_observer_get_feedforward_torque(const pbio_observer_model_t *model, int32_t rate_ref, int32_t acceleration_ref) {
    int32_t friction_compensation_torque = (int32_t)(model->f_low * pbio_math_sign(rate_ref) * 1000000);
    int32_t back_emf_compensation_torque = (int32_t)(model->k_0 * model->k_2 * rate_ref * 1000000);
    int32_t acceleration_torque = (int32_t)(model->k_0 * model->k_1 * acceleration_ref * 1000000);

    // Total feedforward torque
    return (int32_t)(friction_compensation_torque + back_emf_compensation_torque + acceleration_torque);
}

int32_t pbio_observer_torque_to_voltage(const pbio_observer_model_t *model, int32_t desired_torque) {
    return (int32_t)(desired_torque / model->k_0 / 1000);
}

int32_t pbio_observer_voltage_to_torque(const pbio_observer_model_t *model, int32_t voltage) {
    return (int32_t)(model->k_0 * voltage * 1000);
}

#endif // PBIO_CONFIG_CONTROL_MINIMAL
