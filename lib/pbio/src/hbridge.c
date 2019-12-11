// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/config.h>

#if PBIO_CONFIG_HBRIDGE

#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>
#include <pbio/hbridge.h>

static pbio_hbridge_t hbridges[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_error_t pbio_hbridge_setup(pbio_hbridge_t *hbridge, pbio_port_t port, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps) {

    // Set direction and offsets
    hbridge->direction = direction;
    hbridge->duty_offset = duty_offset;
    hbridge->max_duty_steps = max_duty_steps;
    hbridge->port = port;
    hbridge->state = PBIO_HBRIDGE_COAST;

    return pbio_hbridge_coast(hbridge);
}

pbio_error_t pbio_hbridge_get(pbio_port_t port, pbio_hbridge_t **hbridge, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to hbridge
    *hbridge = &hbridges[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];

    // Initialize and set up pwm properties
    return pbio_hbridge_setup(*hbridge, port, direction, duty_offset, max_duty_steps);
}

pbio_error_t pbio_hbridge_set_settings(pbio_hbridge_t *hbridge, int32_t stall_torque_limit_pct, int32_t duty_offset_pct) {
    if (stall_torque_limit_pct < 0 || duty_offset_pct < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    hbridge->max_duty_steps = PBIO_DUTY_STEPS_PER_USER_STEP * stall_torque_limit_pct;
    hbridge->duty_offset = PBIO_DUTY_STEPS_PER_USER_STEP * duty_offset_pct;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_hbridge_get_settings(pbio_hbridge_t *hbridge, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct) {
    *stall_torque_limit_pct = hbridge->max_duty_steps/PBIO_DUTY_STEPS_PER_USER_STEP;
    *duty_offset_pct = hbridge->duty_offset/PBIO_DUTY_STEPS_PER_USER_STEP;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_hbridge_coast(pbio_hbridge_t *hbridge) {
    hbridge->state = PBIO_HBRIDGE_COAST;
    return pbdrv_motor_coast(hbridge->port);
}

pbio_error_t pbio_hbridge_brake(pbio_hbridge_t *hbridge) {
    hbridge->state = PBIO_HBRIDGE_BRAKE;
    return pbdrv_motor_set_duty_cycle(hbridge->port, 0);
}

pbio_error_t pbio_hbridge_set_duty_cycle_sys(pbio_hbridge_t *hbridge, int32_t duty_steps) {

    // Limit the duty cycle value
    int32_t limit = hbridge->max_duty_steps;
    if (duty_steps > limit) {
        duty_steps = limit;
    }
    if (duty_steps < -limit) {
        duty_steps = -limit;
    }

    int32_t duty_cycle;
    // Add the configured offset and scale remaining duty
    if (duty_steps == 0) {
        duty_cycle = 0;
    }
    else {
        int32_t offset = hbridge->duty_offset;
        int32_t offset_signed = duty_steps > 0 ? offset : -offset;
        duty_cycle = offset_signed + ((PBIO_DUTY_STEPS-offset)*duty_steps)/PBIO_DUTY_STEPS;
    }
    // Flip sign if motor is inverted
    if (hbridge->direction == PBIO_DIRECTION_COUNTERCLOCKWISE){
        duty_cycle = -duty_cycle;
    }
    pbio_error_t err = pbdrv_motor_set_duty_cycle(hbridge->port, duty_cycle);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    hbridge->state = PBIO_HBRIDGE_DUTY_ACTIVE;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_hbridge_set_duty_cycle_usr(pbio_hbridge_t *hbridge, int32_t duty_steps) {
    hbridge->state = PBIO_HBRIDGE_DUTY_PASSIVE;
    return pbio_hbridge_set_duty_cycle_sys(hbridge, PBIO_DUTY_STEPS * duty_steps / PBIO_DUTY_USER_STEPS);
}

#endif // PBIO_CONFIG_HBRIDGE
