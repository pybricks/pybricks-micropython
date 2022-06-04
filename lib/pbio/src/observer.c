// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <pbio/math.h>
#include <pbio/observer.h>

// FIXME: For some reason the pbdrv config file for the debug port
// does not pick up this flag. But we are going to drop this flag
// anyway, so use this workaround.
#ifndef PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE
#define PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE (1)
#endif

// FIXME: Use millidegrees consistently throughout the code.
// In the long run, we can improve control performance by
// using units like millidegrees everywhere in order to avoid
// unwanted roundoff. For now, we'll do it only in this module
// and scale appropriately in the setter and getter functions.
#define MDEG_PER_DEG (1000)
#define MDEG_MAX (1000000 * (MDEG_PER_DEG))

void pbio_observer_reset(pbio_observer_t *obs, int32_t count_now) {

    // FIXME: Switch from counts to a consistent unit like millidegrees.
    // Platform-specific scaling like this should happen in pbio/tacho.
    int32_t degrees = count_now / PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE;

    // Initialize the offset to the current position in degrees.
    obs->angle_offset = degrees;
    obs->angle = 0;
    obs->speed = 0;
    obs->current = 0;
}

void pbio_observer_get_estimated_state(pbio_observer_t *obs, int32_t *count, int32_t *rate) {
    *count = obs->angle_offset + obs->angle / MDEG_PER_DEG;
    *rate = obs->speed / MDEG_PER_DEG;
}

void pbio_observer_update(pbio_observer_t *obs, int32_t count, pbio_dcmotor_actuation_t actuation, int32_t voltage) {

    const pbio_observer_model_t *m = obs->model;

    // Convert input angle to millidegrees
    int32_t angle = (count / PBDRV_CONFIG_COUNTER_COUNTS_PER_DEGREE - obs->angle_offset) * MDEG_PER_DEG;

    if (actuation == PBIO_DCMOTOR_ACTUATION_COAST) {
        // TODO
    }

    // Apply observer error feedback as voltage.
    voltage += pbio_observer_torque_to_voltage(m, m->gain * (angle - obs->angle) / MDEG_PER_DEG);

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

    // Reset millidegree value to avoid overflows.
    if (angle_next > MDEG_MAX) {
        angle_next -= MDEG_MAX;
        obs->angle_offset += (MDEG_MAX / MDEG_PER_DEG);
    } else if (angle_next < -MDEG_MAX) {
        angle_next += MDEG_MAX;
        obs->angle_offset -= (MDEG_MAX / MDEG_PER_DEG);
    }

    // Save new state.
    obs->angle = angle_next;
    obs->speed = speed_next;
    obs->current = current_next;
}

int32_t pbio_observer_get_feedforward_torque(const pbio_observer_model_t *model, int32_t rate_ref, int32_t acceleration_ref) {

    // TODO: Adjust units to millidegrees everywhere. Just do it in place for now.
    rate_ref *= MDEG_PER_DEG;
    acceleration_ref *= MDEG_PER_DEG;

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
