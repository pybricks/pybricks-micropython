// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Driver for 2 individual RGB LEDs that function as a single LED.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_DUAL

#include <stdint.h>

#include <pbdrv/led.h>
#include <pbdrv/pwm.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/math.h>

#include "led_dual.h"
#include "led.h"

#ifndef PBDRV_CONFIG_LED_DUAL_NUM_DEV
#error "Must define PBDRV_CONFIG_LED_DUAL_NUM_DEV"
#endif

static pbio_error_t pbdrv_led_dual_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv) {
    const pbdrv_led_dual_platform_data_t *pdata = dev->pdata;
    pbdrv_led_dev_t *led;

    // REVISIT: may want to propagate error

    if (pbdrv_led_get_dev(pdata->id1, &led) == PBIO_SUCCESS) {
        pbdrv_led_set_hsv(led, hsv);
    }
    if (pbdrv_led_get_dev(pdata->id2, &led) == PBIO_SUCCESS) {
        pbdrv_led_set_hsv(led, hsv);
    }

    return PBIO_SUCCESS;
}

static const pbdrv_led_funcs_t pbdrv_led_dual_funcs = {
    .set_hsv = pbdrv_led_dual_set_hsv,
};

void pbdrv_led_dual_init(pbdrv_led_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_LED_DUAL_NUM_DEV; i++) {
        const pbdrv_led_dual_platform_data_t *pdata = &pbdrv_led_dual_platform_data[i];
        devs[pdata->id].pdata = pdata;
        devs[pdata->id].funcs = &pbdrv_led_dual_funcs;
    }
}

#endif // PBDRV_CONFIG_LED_DUAL
