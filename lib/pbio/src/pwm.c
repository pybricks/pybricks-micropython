// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <inttypes.h>

#include <fixmath.h>

#include <pbio/port.h>
#include <pbio/pwm.h>

static pbio_pwm_t pwms[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

static pbio_error_t pbio_pwm_setup(pbio_pwm_t *pwm, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps) {

    // Set direction and offsets
    pwm->direction = direction;
    pwm->duty_offset = duty_offset;
    pwm->max_duty_steps = max_duty_steps;

    return PBIO_SUCCESS;// TODO: return coast
}

pbio_error_t pbio_pwm_get(pbio_port_t port, pbio_pwm_t **pwm, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    // Get pointer to pwm
    *pwm = &pwms[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    
    // Initialize and set up pwm properties
    return pbio_pwm_setup(*pwm, direction, duty_offset, max_duty_steps);
}
