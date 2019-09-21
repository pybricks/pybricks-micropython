// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_HBRIDGE_H_
#define _PBIO_HBRIDGE_H_

#include <stdint.h>
#include <pbio/port.h>

#define PBIO_DUTY_STEPS (PBDRV_MAX_DUTY)
#define PBIO_DUTY_USER_STEPS (100)
#define PBIO_DUTY_STEPS_PER_USER_STEP (PBIO_DUTY_STEPS/PBIO_DUTY_USER_STEPS)

typedef enum {
    PBIO_DIRECTION_CLOCKWISE,         /**< Positive means clockwise */
    PBIO_DIRECTION_COUNTERCLOCKWISE,  /**< Positive means counterclockwise */
} pbio_direction_t;

typedef enum {
    PBIO_HBRIDGE_COAST,               /**< PWM is (and remains) in coast mode. */
    PBIO_HBRIDGE_BRAKE,               /**< PWM is (and remains) in brake mode. */
    PBIO_HBRIDGE_PASSIVE,             /**< PWM is (and remains) in constant duty. */
    PBIO_HBRIDGE_ACTIVE,              /**< PWM device is claimed by some higher level controller. */
} pbio_passivity_t;

typedef struct _pbio_hbridge_t {
    pbio_port_t port;
    pbio_direction_t direction;
    int32_t duty_offset;
    int32_t max_duty_steps;
    pbio_passivity_t state;
} pbio_hbridge_t;

pbio_error_t pbio_hbridge_get(pbio_port_t port, pbio_hbridge_t **hbridge, pbio_direction_t direction, int32_t duty_offset, int32_t max_duty_steps);

pbio_error_t pbio_hbridge_set_settings(pbio_hbridge_t *hbridge, int32_t stall_torque_limit_pct, int32_t duty_offset_pct);
pbio_error_t pbio_hbridge_get_settings(pbio_hbridge_t *hbridge, int32_t *stall_torque_limit_pct, int32_t *duty_offset_pct);

pbio_error_t pbio_hbridge_coast(pbio_hbridge_t *hbridge);
pbio_error_t pbio_hbridge_brake(pbio_hbridge_t *hbridge);
pbio_error_t pbio_hbridge_set_duty_cycle_sys(pbio_hbridge_t *hbridge, int32_t duty_steps);
pbio_error_t pbio_hbridge_set_duty_cycle_usr(pbio_hbridge_t *hbridge, int32_t duty_steps);

#endif // _PBIO_HBRIDGE_H_
