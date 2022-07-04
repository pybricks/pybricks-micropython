// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_OBSERVER_H_
#define _PBIO_OBSERVER_H_

#include <stdint.h>

#include <pbio/dcmotor.h>
#include <pbio/angle.h>

/**
 * Device-type specific constants that describe the motor model.
 */
typedef struct _pbio_observer_model_t {
    int32_t d_angle_d_speed;
    int32_t d_speed_d_speed;
    int32_t d_current_d_speed;
    int32_t d_angle_d_current;
    int32_t d_speed_d_current;
    int32_t d_current_d_current;
    int32_t d_angle_d_voltage;
    int32_t d_speed_d_voltage;
    int32_t d_current_d_voltage;
    int32_t d_angle_d_torque;
    int32_t d_speed_d_torque;
    int32_t d_current_d_torque;
    int32_t d_voltage_d_torque;
    int32_t d_torque_d_voltage;
    int32_t d_torque_d_speed;
    int32_t d_torque_d_acceleration;
    int32_t torque_friction;
    /**
     * Control gain to correct the observer for a given estimation error.
     */
    int32_t gain;
} pbio_observer_model_t;

/**
 * Motor state observer object.
 */
typedef struct _pbio_observer_t {
    /**
     * Angle state of observer (estimated system angle) in millidegrees
     */
    pbio_angle_t angle;
    /**
     * Speed state of observer (estimated system speed) in millidegrees/second.
     */
    int32_t speed;
    /**
     * Current state of observer (estimated system current) in tenths of milliAmperes: 10000 = 1A.
     */
    int32_t current;
    /**
     * Whether the motor is stalled according to the model.
     */
    bool stalled;
    /**
     * If stalled, this is the time that stall was first detected.
     */
    uint32_t stall_start;
    /**
     * Model parameters used by this model.
     */
    const pbio_observer_model_t *model;
} pbio_observer_t;

void pbio_observer_reset(pbio_observer_t *obs, pbio_angle_t *angle);

void pbio_observer_get_estimated_state(pbio_observer_t *obs, pbio_angle_t *angle, int32_t *speed);

void pbio_observer_update(pbio_observer_t *obs, uint32_t time, pbio_angle_t *angle, pbio_dcmotor_actuation_t actuation, int32_t voltage);

bool pbio_observer_is_stalled(pbio_observer_t *obs, uint32_t time, uint32_t stall_threshold, uint32_t *stall_duration);

int32_t pbio_observer_get_feedforward_torque(const pbio_observer_model_t *model, int32_t rate_ref, int32_t acceleration_ref);

int32_t pbio_observer_torque_to_voltage(const pbio_observer_model_t *model, int32_t desired_torque);

int32_t pbio_observer_voltage_to_torque(const pbio_observer_model_t *model, int32_t voltage);

#endif // _PBIO_OBSERVER_H_
