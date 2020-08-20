// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup PWMDriver Pulse width modulation (PWM) I/O driver
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

/**
 * Driver callback functions.
 */
typedef struct {
    /** Driver implementation of pbdrv_pwm_set_duty() */
    pbio_error_t (*set_duty)(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value);
} pbdrv_pwm_driver_funcs_t;

struct _pbdrv_pwm_dev_t {
    /** Platform data. */
    const void *pdata;
    /** Driver callback functions. */
    const pbdrv_pwm_driver_funcs_t *funcs;
    /** Private driver data */
    void *priv;
};

#if PBDRV_CONFIG_PWM

void pbdrv_pwm_init();
pbio_error_t pbdrv_pwm_get_dev(uint8_t id, pbdrv_pwm_dev_t **dev);
pbio_error_t pbdrv_pwm_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value);

#else

#define pbdrv_pwm_init()

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
