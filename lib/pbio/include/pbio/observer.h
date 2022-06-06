// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_OBSERVER_H_
#define _PBIO_OBSERVER_H_

#include <stdint.h>

#include <pbio/control.h>


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
    int32_t d_torque_d_voltage;
    int32_t d_torque_d_speed;
    int32_t d_torque_d_acceleration;
    int32_t torque_friction;
    int32_t gain;
} pbio_observer_model_t;

typedef struct _pbio_observer_t {
    int32_t angle;        /**< Angle in millidegrees. */
    int32_t angle_offset; /**< Angle offset to prevent millidegree buffer from overflowing. */
    int32_t speed;        /**< Speed in millidegrees per secon. */
    int32_t current;      /**< Current in tenths of milliAmperes: 10000 = 1A */
    bool stalled;         /**< Whether the motor is stalled according to model. */
    int32_t stall_start;  /**< Time at which stall was first detected. */
    const pbio_observer_model_t *model;
} pbio_observer_t;

void pbio_observer_reset(pbio_observer_t *obs, int32_t count_now);

void pbio_observer_get_estimated_state(pbio_observer_t *obs, int32_t *count, int32_t *rate);

void pbio_observer_update(pbio_observer_t *obs, int32_t time, int32_t count, pbio_dcmotor_actuation_t actuation, int32_t voltage);

bool pbio_observer_is_stalled(pbio_observer_t *obs, int32_t time, int32_t *stall_duration);

int32_t pbio_observer_get_feedforward_torque(const pbio_observer_model_t *model, int32_t rate_ref, int32_t acceleration_ref);

int32_t pbio_observer_torque_to_voltage(const pbio_observer_model_t *model, int32_t desired_torque);

int32_t pbio_observer_voltage_to_torque(const pbio_observer_model_t *model, int32_t voltage);

#endif // _PBIO_OBSERVER_H_
