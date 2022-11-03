// SPDX-License-Identifier: MIT
// Copyright (c) 2020,2022 The Pybricks Authors

// Driver for PWM-driven RGB LED Devices.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_PWM

#include <stdint.h>

#include <pbdrv/led.h>
#include <pbdrv/pwm.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/int_math.h>

#include "led_pwm.h"
#include "led.h"

#ifndef PBDRV_CONFIG_LED_PWM_NUM_DEV
#error "Must define PBDRV_CONFIG_LED_PWM_NUM_DEV"
#endif

static pbio_error_t pbdrv_led_pwm_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv) {
    const pbdrv_led_pwm_platform_data_t *pdata = dev->pdata;
    const pbdrv_led_pwm_platform_color_t *color = pdata->color;

    // copy of HSV with max V for best color match
    pbio_color_hsv_t hsv2 = {
        .h = hsv->h,
        .s = hsv->s,
        .v = 100,
    };
    pbio_color_rgb_t rgb;
    pbio_color_hsv_to_rgb(&hsv2, &rgb);

    // Adjust for chromacity
    uint32_t r = rgb.r * color->r_factor;
    uint32_t g = rgb.g * color->g_factor;
    uint32_t b = rgb.b * color->b_factor;

    // Adjust for apparent brightness

    // Right-shift avoids need for large scale factor that would overflow 32-bit
    // integer. Plus 1 avoids divide by 0.
    uint32_t Y = ((color->r_brightness * r + color->g_brightness * g + color->b_brightness * b) >> 12) + 1;

    // Reapply V from HSV for brightness. V is squared for gamma correction.
    uint32_t scale_factor = hsv->v * hsv->v * pdata->scale_factor / 128;

    r = r * scale_factor / Y;
    g = g * scale_factor / Y;
    b = b * scale_factor / Y;

    pbdrv_pwm_dev_t *pwm;
    if (pbdrv_pwm_get_dev(pdata->r_id, &pwm) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm, pdata->r_ch, r);
    }
    if (pbdrv_pwm_get_dev(pdata->g_id, &pwm) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm, pdata->g_ch, g);
    }
    if (pbdrv_pwm_get_dev(pdata->b_id, &pwm) == PBIO_SUCCESS) {
        pbdrv_pwm_set_duty(pwm, pdata->b_ch, b);
    }

    return PBIO_SUCCESS;
}

static const pbdrv_led_funcs_t pbdrv_led_pwm_funcs = {
    .set_hsv = pbdrv_led_pwm_set_hsv,
};

void pbdrv_led_pwm_init(pbdrv_led_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_LED_PWM_NUM_DEV; i++) {
        const pbdrv_led_pwm_platform_data_t *pdata = &pbdrv_led_pwm_platform_data[i];
        devs[pdata->id].pdata = pdata;
        devs[pdata->id].funcs = &pbdrv_led_pwm_funcs;
    }
}

#endif // PBDRV_CONFIG_LED_PWM
