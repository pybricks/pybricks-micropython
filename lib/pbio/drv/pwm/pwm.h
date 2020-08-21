// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Pulse width modulation (PWM) drivers

#ifndef _PBDRV_PWM_PWM_H_
#define _PBDRV_PWM_PWM_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbdrv/pwm.h>
#include <pbio/error.h>

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

#else // PBDRV_CONFIG_PWM

#define pbdrv_pwm_init()

#endif // PBDRV_CONFIG_PWM

#endif // _PBDRV_PWM_PWM_H_
