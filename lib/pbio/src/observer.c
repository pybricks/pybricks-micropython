// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <pbio/math.h>
#include <pbio/observer.h>

void pbio_observer_reset(pbio_observer_t *obs, int32_t count_now, int32_t rate_now) {
    // FIXME: EV3 counts
    obs->angle = count_now * 1000;
    obs->speed = rate_now * 1000;
    obs->current = 0;
}

void pbio_observer_get_estimated_state(pbio_observer_t *obs, int32_t *count, int32_t *rate) {
    *count = obs->angle / 1000;
    *rate = obs->speed / 1000;
}

void pbio_observer_update(pbio_observer_t *obs, int32_t count, pbio_dcmotor_actuation_t actuation, int32_t voltage) {

    const pbio_observer_model_t *m = obs->model;

    // FIXME, convert units throughout, then delete this.
    int32_t angle = count * 1000;

    if (actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        // TODO
    }

    // Apply observer error feedback as voltage.
    voltage += pbio_observer_torque_to_voltage(m, m->gain * (angle - obs->angle) / 1000);

    int32_t torque = obs->speed > 0 ? m->torque_friction: -m->torque_friction;

    // Get next state based on current state and input: x(k+1) = Ax(k) + Bu(k)
    int32_t angle_next = obs->angle +
        PRESCALE_SPEED * obs->speed / m->d_angle_d_speed +
        PRESCALE_CURRENT * obs->current / m->d_angle_d_current +
        PRESCALE_VOLTAGE * voltage / m->d_angle_d_voltage +
        PRESCALE_TORQUE * torque / m->d_angle_d_torque;
    int32_t speed_next = 0 +
        PRESCALE_SPEED * obs->speed / m->d_speed_d_speed +
        PRESCALE_CURRENT * obs->current / m->d_speed_d_current +
        PRESCALE_VOLTAGE * voltage / m->d_speed_d_voltage +
        PRESCALE_TORQUE * torque / m->d_speed_d_torque;
    int32_t current_next = 0 +
        PRESCALE_SPEED * obs->speed / m->d_current_d_speed +
        PRESCALE_CURRENT * obs->current / m->d_current_d_current +
        PRESCALE_VOLTAGE * voltage / m->d_current_d_voltage +
        PRESCALE_TORQUE * torque / m->d_current_d_torque;

    // TODO: Better friction model.
    if ((speed_next < 0) != (speed_next - PRESCALE_TORQUE * torque / m->d_speed_d_torque < 0)) {
        speed_next = 0;
    }

    // Save new state.
    obs->angle = angle_next;
    obs->speed = speed_next;
    obs->current = current_next;
}

int32_t pbio_observer_get_feedforward_torque(const pbio_observer_model_t *model, int32_t rate_ref, int32_t acceleration_ref) {

    // TODO: Adjust units to millidegrees everywhere. Just do it in place for now.
    rate_ref *= 1000;
    acceleration_ref *= 1000;

    int32_t friction_compensation_torque = model->torque_friction * pbio_math_sign(rate_ref);
    int32_t back_emf_compensation_torque = PRESCALE_SPEED * rate_ref / model->d_torque_d_speed;
    int32_t acceleration_torque = PRESCALE_ACCELERATION * acceleration_ref / model->d_torque_d_acceleration;

    // Total feedforward torque
    return friction_compensation_torque + back_emf_compensation_torque + acceleration_torque;
}

int32_t pbio_observer_torque_to_voltage(const pbio_observer_model_t *model, int32_t desired_torque) {
    return desired_torque * model->d_torque_d_voltage / PRESCALE_VOLTAGE;
}

int32_t pbio_observer_voltage_to_torque(const pbio_observer_model_t *model, int32_t voltage) {
    return PRESCALE_VOLTAGE * voltage / model->d_torque_d_voltage;
}
