// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <inttypes.h>

#include <fixmath.h>

#include <pbdrv/motor.h>

#include <pbio/pwm.h>

static pbio_pwm_t pwms[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_error_t pbio_pwm_setup(pbio_pwm_t *pwm, pbio_port_t port, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps) {

    // Set direction and offsets
    pwm->direction = direction;
    pwm->duty_offset = duty_offset;
    pwm->max_duty_steps = max_duty_steps;
    pwm->port = port;

    return pbio_pwm_coast(pwm);
}

pbio_error_t pbio_pwm_get(pbio_port_t port, pbio_pwm_t **pwm, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to pwm
    *pwm = &pwms[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    
    // Initialize and set up pwm properties
    return pbio_pwm_setup(*pwm, port, direction, duty_offset, max_duty_steps);
}

pbio_error_t pbio_pwm_coast(pbio_pwm_t *pwm) {
    pwm->state = PBIO_PWM_COAST;
    return pbdrv_motor_coast(pwm->port);
}

pbio_error_t pbio_pwm_brake(pbio_pwm_t *pwm) {
    pwm->state = PBIO_PWM_BRAKE;
    return pbdrv_motor_set_duty_cycle(pwm->port, 0);
}

pbio_error_t pbio_pwm_set_duty_cycle_sys(pbio_pwm_t *pwm, int32_t duty_steps) {

    // Limit the duty cycle value
    int32_t limit = pwm->max_duty_steps;
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
        int32_t offset = pwm->duty_offset;
        int32_t offset_signed = duty_steps > 0 ? offset : -offset;
        duty_cycle = offset_signed + ((PBIO_DUTY_STEPS-offset)*duty_steps)/PBIO_DUTY_STEPS;
    }
    // Flip sign if motor is inverted
    if (pwm->direction == PBIO_DIRECTION_COUNTERCLOCKWISE){
        duty_cycle = -duty_cycle;
    }
    pbio_error_t err = pbdrv_motor_set_duty_cycle(pwm->port, duty_cycle);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    pwm->state = PBIO_PWM_ACTIVE;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_pwm_set_duty_cycle_usr(pbio_pwm_t *pwm, int32_t duty_steps) {
    pwm->state = PBIO_PWM_PASSIVE;
    return pbio_pwm_set_duty_cycle_sys(pwm, PBIO_DUTY_STEPS * duty_steps / PBIO_DUTY_USER_STEPS);
}
