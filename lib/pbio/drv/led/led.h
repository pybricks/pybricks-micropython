// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// LED driver internal types and functions

#ifndef _INTERNAL_PBDRV_LED_H_
#define _INTERNAL_PBDRV_LED_H_

#include <pbdrv/config.h>
#include <pbdrv/led.h>
#include <pbio/color.h>
#include <pbio/error.h>

typedef struct {
    pbio_error_t (*set_hsv)(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv);
} pbdrv_led_funcs_t;

/** LED device instance. */
struct _pbdrv_led_dev_t {
    /** Platform-specific data */
    const void *pdata;
    /** Driver-specific callbacks. */
    const pbdrv_led_funcs_t *funcs;
};

#if PBDRV_CONFIG_LED

void pbdrv_led_init(void);

#else // PBDRV_CONFIG_LED

#define pbdrv_led_init()

#endif // PBDRV_CONFIG_LED

#endif // _INTERNAL_PBDRV_LED_H_
