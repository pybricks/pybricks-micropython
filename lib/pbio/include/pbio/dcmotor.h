// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBIO_DCMOTOR_H_
#define _PBIO_DCMOTOR_H_

#include <stdint.h>
#include <pbio/config.h>
#include <pbio/iodev.h>
#include <pbio/port.h>

typedef enum {
    PBIO_DIRECTION_CLOCKWISE,         /**< Positive means clockwise */
    PBIO_DIRECTION_COUNTERCLOCKWISE,  /**< Positive means counterclockwise */
} pbio_direction_t;

#define PBIO_DUTY_USER_STEPS (100)

typedef enum {
    PBIO_DCMOTOR_COAST,               /**< dcmotor set to coast */
    PBIO_DCMOTOR_BRAKE,               /**< dcmotor set to brake */
    PBIO_DCMOTOR_DUTY_PASSIVE,        /**< dcmotor set to constant duty. */
    PBIO_DCMOTOR_CLAIMED,             /**< dcmotor set to varying duty by active controller. */
} pbio_passivity_t;

typedef struct _pbio_dcmotor_t {
    pbio_port_t port;
    pbio_iodev_type_id_t id;
    pbio_direction_t direction;
    pbio_passivity_t state;
    int32_t duty_now;
} pbio_dcmotor_t;

#if PBIO_CONFIG_DCMOTOR

pbio_error_t pbio_dcmotor_get(pbio_port_t port, pbio_dcmotor_t **dcmotor, pbio_direction_t direction, bool is_servo);

pbio_error_t pbio_dcmotor_get_state(pbio_dcmotor_t *dcmotor, pbio_passivity_t *state, int32_t *duty_now);

pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor);
pbio_error_t pbio_dcmotor_brake(pbio_dcmotor_t *dcmotor);
pbio_error_t pbio_dcmotor_set_duty_cycle_sys(pbio_dcmotor_t *dcmotor, int32_t duty_steps);
pbio_error_t pbio_dcmotor_set_duty_cycle_usr(pbio_dcmotor_t *dcmotor, int32_t duty_steps);

#else

static inline pbio_error_t pbio_dcmotor_get(pbio_port_t port, pbio_dcmotor_t **dcmotor, pbio_direction_t direction, bool is_servo) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_dcmotor_get_state(pbio_dcmotor_t *dcmotor, pbio_passivity_t *state, int32_t *duty_now) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_dcmotor_coast(pbio_dcmotor_t *dcmotor) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_dcmotor_brake(pbio_dcmotor_t *dcmotor) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_dcmotor_set_duty_cycle_sys(pbio_dcmotor_t *dcmotor, int32_t duty_steps) {
    return PBIO_ERROR_NOT_SUPPORTED;
}
static inline pbio_error_t pbio_dcmotor_set_duty_cycle_usr(pbio_dcmotor_t *dcmotor, int32_t duty_steps) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBIO_CONFIG_DCMOTOR

#endif // _PBIO_DCMOTOR_H_
