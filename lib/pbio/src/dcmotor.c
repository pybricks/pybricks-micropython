// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/config.h>

#if PBIO_CONFIG_DCMOTOR

#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>
#include <pbio/dcmotor.h>

static pbio_dcmotor_t dcmotors[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, pbio_direction_t direction) {

    pbio_error_t err;

    // Coast the device
    err = pbio_dcmotor_coast(dcmotor);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get device ID to ensure we are dealing with a supported device
    err = pbdrv_motor_get_id(dcmotor->port, &dcmotor->id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set direction and state
    dcmotor->direction = direction;
    dcmotor->state = PBIO_DCMOTOR_COAST;

    // Set duty scaling and offsets
    return pbio_dcmotor_set_settings(dcmotor,  100, 0);
}

pbio_error_t pbio_dcmotor_get(pbio_port_t port, pbio_dcmotor_t **dcmotor, pbio_direction_t direction) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to dcmotor
    *dcmotor = &dcmotors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    (*dcmotor)->port = port;

    // Initialize and set up pwm properties
    return pbio_dcmotor_setup(*dcmotor, direction);
}

pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t stall_torque_limit_pct, int32_t duty_offset_pct) {
    if (stall_torque_limit_pct < 0 || duty_offset_pct < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    dcmotor->max_duty_steps = PBIO_DUTY_STEPS_PER_USER_STEP * stall_torque_limit_pct;
    dcmotor->duty_offset = PBIO_DUTY_STEPS_PER_USER_STEP * duty_offset_pct;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_get_settings(pbio_dcmotor_t *dcmotor, pbio_direction_t *direction, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct) {
    *direction = dcmotor->direction;
    *stall_torque_limit_pct = dcmotor->max_duty_steps/PBIO_DUTY_STEPS_PER_USER_STEP;
    *duty_offset_pct = dcmotor->duty_offset/PBIO_DUTY_STEPS_PER_USER_STEP;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_get_state(pbio_dcmotor_t *dcmotor, pbio_passivity_t *state, int32_t *duty_now) {
    *state = dcmotor->state;
    *duty_now = dcmotor->state < PBIO_DCMOTOR_DUTY_PASSIVE ? 0 : dcmotor->duty_now;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor) {
    dcmotor->state = PBIO_DCMOTOR_COAST;
    return pbdrv_motor_coast(dcmotor->port);
}

pbio_error_t pbio_dcmotor_brake(pbio_dcmotor_t *dcmotor) {
    dcmotor->state = PBIO_DCMOTOR_BRAKE;
    return pbdrv_motor_set_duty_cycle(dcmotor->port, 0);
}

pbio_error_t pbio_dcmotor_set_duty_cycle_sys(pbio_dcmotor_t *dcmotor, int32_t duty_steps) {

    // Limit the duty cycle value
    int32_t limit = dcmotor->max_duty_steps;
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
        int32_t offset = dcmotor->duty_offset;
        int32_t offset_signed = duty_steps > 0 ? offset : -offset;
        duty_cycle = offset_signed + ((PBIO_DUTY_STEPS-offset)*duty_steps)/PBIO_DUTY_STEPS;
    }
    // Signed duty cycle applied to bridge
    dcmotor->duty_now = duty_cycle;

    // Flip sign if motor is inverted
    if (dcmotor->direction == PBIO_DIRECTION_COUNTERCLOCKWISE){
        duty_cycle = -duty_cycle;
    }
    pbio_error_t err = pbdrv_motor_set_duty_cycle(dcmotor->port, duty_cycle);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    dcmotor->state = PBIO_DCMOTOR_CLAIMED;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_set_duty_cycle_usr(pbio_dcmotor_t *dcmotor, int32_t duty_steps) {
    pbio_error_t err = pbio_dcmotor_set_duty_cycle_sys(dcmotor,  PBIO_DUTY_STEPS * duty_steps / PBIO_DUTY_USER_STEPS);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    dcmotor->state = PBIO_DCMOTOR_DUTY_PASSIVE;
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_DCMOTOR
