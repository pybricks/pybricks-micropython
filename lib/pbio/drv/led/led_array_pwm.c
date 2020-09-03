// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Driver for PWM-driven LED arrays.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_ARRAY_PWM

#include <stdint.h>

#include <pbdrv/led.h>
#include <pbdrv/pwm.h>

#include <pbio/error.h>

#include "led_array_pwm.h"
#include "led_array.h"

#ifndef PBDRV_CONFIG_LED_ARRAY_PWM_NUM_DEV
#error "Must define PBDRV_CONFIG_LED_ARRAY_PWM_NUM_DEV"
#endif

static pbio_error_t pbdrv_led_array_pwm_set_brightness(pbdrv_led_array_dev_t *dev, uint8_t index, uint8_t brightness) {
    const pbdrv_led_array_pwm_platform_data_t *pdata = dev->pdata;

    if (index >= pdata->num_pwm_chs) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // REVISIT: currently all known devices have PWM period of UINT16_MAX, so
    // we scale accordingly. Scaling can be added to the platform data in the
    // future if needed.
    // Brightness is squared for gamma correction.
    uint32_t duty = UINT16_MAX * brightness * brightness / 10000;

    pbdrv_pwm_dev_t *pwm;
    if (pbdrv_pwm_get_dev(pdata->pwm_id, &pwm) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm, pdata->pwm_chs[index], duty);
    }

    return PBIO_SUCCESS;
}

static const pbdrv_led_array_funcs_t pbdrv_led_array_pwm_funcs = {
    .set_brightness = pbdrv_led_array_pwm_set_brightness,
};

void pbdrv_led_array_pwm_init(pbdrv_led_array_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_LED_ARRAY_PWM_NUM_DEV; i++) {
        const pbdrv_led_array_pwm_platform_data_t *pdata = &pbdrv_led_array_pwm_platform_data[i];
        devs[pdata->id].pdata = pdata;
        devs[pdata->id].funcs = &pbdrv_led_array_pwm_funcs;
    }
}

#endif // PBDRV_CONFIG_LED_ARRAY_PWM
