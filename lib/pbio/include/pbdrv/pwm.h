// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup PWMDriver Driver: Pulse width modulation (PWM)
 * @{
 */

#ifndef _PBDRV_PWM_H_
#define _PBDRV_PWM_H_

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>

/**
 * PWM device.
 */
typedef struct _pbdrv_pwm_dev_t pbdrv_pwm_dev_t;

#if PBDRV_CONFIG_PWM

pbio_error_t pbdrv_pwm_get_dev(uint8_t id, pbdrv_pwm_dev_t **dev);
pbio_error_t pbdrv_pwm_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value);

#else

static inline pbio_error_t pbdrv_pwm_get_dev(uint8_t id, pbdrv_pwm_dev_t **dev) {
    *dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_pwm_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif /* _PBDRV_PWM_H_ */

/** @} */
