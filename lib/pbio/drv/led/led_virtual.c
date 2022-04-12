// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

// Virtual light that is implemented in Python.

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_VIRTUAL

#include <Python.h>

#include <pbdrv/led.h>

#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/util.h>

#include "led.h"
#include "../virtual.h"

#ifndef PBDRV_CONFIG_LED_VIRTUAL_NUM_DEV
#error "Must define PBDRV_CONFIG_LED_VIRTUAL_NUM_DEV"
#endif

static pbio_error_t pbdrv_led_virtual_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv) {
    uint8_t id = (intptr_t)dev->pdata;

    pbio_color_rgb_t rgb;
    pbio_color_hsv_to_rgb(hsv, &rgb);

    return pbdrv_virtual_platform_call_method("on_light", "BBBB", id, rgb.r, rgb.g, rgb.b);
}

static const pbdrv_led_funcs_t pbdrv_led_virtual_funcs = {
    .set_hsv = pbdrv_led_virtual_set_hsv,
};

void pbdrv_led_virtual_init(pbdrv_led_dev_t *devs) {
    for (int i = 0; i < PBDRV_CONFIG_LED_VIRTUAL_NUM_DEV; i++) {
        // HACK: this assumes that there are no other LEDs so i == global index
        #if PBDRV_CONFIG_LED_VIRTUAL_NUM_DEV != PBDRV_CONFIG_LED_VIRTUAL_NUM_DEV
        #error "need to change code to provide platform data with id"
        #endif
        devs[i].pdata = (const void *)(intptr_t)i;
        devs[i].funcs = &pbdrv_led_virtual_funcs;
    }
}

#endif // PBDRV_CONFIG_LED_VIRTUAL
