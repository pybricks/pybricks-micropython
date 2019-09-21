// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_PWM_H_
#define _PBIO_PWM_H_

#include <stdint.h>

#define PBIO_DUTY_STEPS (PBDRV_MAX_DUTY)
#define PBIO_DUTY_USER_STEPS (100)
#define PBIO_DUTY_STEPS_PER_USER_STEP (PBIO_DUTY_STEPS/PBIO_DUTY_USER_STEPS)

typedef enum {
    PBIO_DIRECTION_CLOCKWISE,         /**< Positive means clockwise */
    PBIO_DIRECTION_COUNTERCLOCKWISE,  /**< Positive means counterclockwise */
} pbio_direction_t;

typedef enum {
    PBIO_PWM_COAST,               /**< PWM is (and remains) in coast mode. */
    PBIO_PWM_BRAKE,               /**< PWM is (and remains) in brake mode. */
    PBIO_PWM_PASSIVE,             /**< PWM is (and remains) in constant duty. */
    PBIO_PWM_ACTIVE,              /**< PWM device is claimed by some higher level controller. */
} pbio_passivity_t;

typedef struct _pbio_pwm_t {
    pbio_port_t port;
    pbio_direction_t direction;
    int32_t duty_offset;
    int32_t max_duty_steps;
    pbio_passivity_t state;
} pbio_pwm_t;

pbio_error_t pbio_pwm_get(pbio_port_t port, pbio_pwm_t **pwm, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps);

pbio_error_t pbio_pwm_coast(pbio_pwm_t *pwm);
pbio_error_t pbio_pwm_brake(pbio_pwm_t *pwm);
pbio_error_t pbio_pwm_set_duty_cycle_sys(pbio_pwm_t *pwm, int32_t duty_steps);
pbio_error_t pbio_pwm_set_duty_cycle_usr(pbio_pwm_t *pwm, int32_t duty_steps);

#endif // _PBIO_PWM_H_
