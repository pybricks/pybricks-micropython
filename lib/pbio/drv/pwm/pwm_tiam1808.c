// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_PWM_TIAM1808

#include <stdio.h>

#include <pbdrv/pwm.h>
#include <pbdrv/gpio.h>
#include <pbio/error.h>

#include "../drv/pwm/pwm.h"
#include "../../drv/pwm/pwm_tiam1808.h"

static pbio_error_t pbdrv_pwm_tiam1808_set_duty(pbdrv_pwm_dev_t *dev, uint32_t ch, uint32_t value) {
    // Blue not available.
    if (ch == 2) {
        return PBIO_SUCCESS;
    }
    pbdrv_pwm_tiam1808_platform_data_t *priv = dev->priv;

    // TODO: implement PWM. Just use GPIO for now.
    pbdrv_gpio_t *gpio = (ch == 0) ? &priv->gpio_red : &priv->gpio_green;
    if (value) {
        pbdrv_gpio_out_high(gpio);
    } else {
        pbdrv_gpio_out_low(gpio);
    }
    return PBIO_SUCCESS;
}

static const pbdrv_pwm_driver_funcs_t pbdrv_pwm_tiam1808_funcs = {
    .set_duty = pbdrv_pwm_tiam1808_set_duty,
};

void pbdrv_pwm_tiam1808_init(pbdrv_pwm_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_PWM_TIAM1808_NUM_DEV; i++) {
        devs[i].funcs = &pbdrv_pwm_tiam1808_funcs;
        devs[i].priv = (pbdrv_pwm_tiam1808_platform_data_t *)&pbdrv_pwm_tiam1808_platform_data[i];

        // Turn on green LED on boot until we have a dedicated HMI UI.
        // pbdrv_pwm_tiam1808_set_duty(&devs[i], 0, 1);
    }
}

#endif // PBDRV_CONFIG_PWM_TIAM1808
