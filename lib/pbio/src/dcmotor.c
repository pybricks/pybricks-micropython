// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_DCMOTOR

#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/config.h>
#include <pbdrv/motor.h>
#include <pbio/dcmotor.h>

static pbio_dcmotor_t dcmotors[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_error_t pbio_dcmotor_setup(pbio_dcmotor_t *dcmotor, pbio_direction_t direction, bool is_servo) {

    pbio_error_t err;

    // Configure up motor ports if needed
    err = pbdrv_motor_setup(dcmotor->port, is_servo);
    if (err != PBIO_SUCCESS) {
        return err;
    }

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

    // Load settings for this motor
    err = pbio_dcmotor_load_settings(dcmotor, dcmotor->id);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Set direction and state
    dcmotor->direction = direction;
    dcmotor->state = PBIO_DCMOTOR_COAST;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_get(pbio_port_id_t port, pbio_dcmotor_t **dcmotor, pbio_direction_t direction, bool is_servo) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to dcmotor
    *dcmotor = &dcmotors[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    (*dcmotor)->port = port;

    // Initialize and set up pwm properties
    return pbio_dcmotor_setup(*dcmotor, direction, is_servo);
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
    int32_t limit = PBDRV_MAX_DUTY;
    if (duty_steps > limit) {
        duty_steps = limit;
    }
    if (duty_steps < -limit) {
        duty_steps = -limit;
    }

    // Signed duty cycle applied to bridge
    dcmotor->duty_now = duty_steps;

    // Flip sign if motor is inverted
    if (dcmotor->direction == PBIO_DIRECTION_COUNTERCLOCKWISE) {
        duty_steps = -duty_steps;
    }
    pbio_error_t err = pbdrv_motor_set_duty_cycle(dcmotor->port, duty_steps);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    dcmotor->state = PBIO_DCMOTOR_CLAIMED;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_dcmotor_set_duty_cycle_usr(pbio_dcmotor_t *dcmotor, int32_t duty_steps) {
    pbio_error_t err = pbio_dcmotor_set_duty_cycle_sys(dcmotor, duty_steps);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    dcmotor->state = PBIO_DCMOTOR_DUTY_PASSIVE;
    return PBIO_SUCCESS;
}

void pbio_dcmotor_get_settings(pbio_dcmotor_t *dcmotor, int32_t *max_voltage) {
    *max_voltage = dcmotor->max_voltage;
}

pbio_error_t pbio_dcmotor_set_settings(pbio_dcmotor_t *dcmotor, int32_t max_voltage) {
    dcmotor->max_voltage = max_voltage;
    return PBIO_SUCCESS;
}

#endif // PBIO_CONFIG_DCMOTOR
