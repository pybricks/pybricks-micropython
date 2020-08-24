// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup LEDDriver Driver: Light Emitting Diode (LED)
 * @{
 */

#ifndef _PBDRV_LED_H_
#define _PBDRV_LED_H_

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/config.h>

#include <pbio/color.h>
#include <pbio/error.h>

typedef struct _pbdrv_led_dev_t pbdrv_led_dev_t;

#if PBDRV_CONFIG_LED

pbio_error_t pbdrv_led_get_dev(uint8_t id, pbdrv_led_dev_t **dev);
pbio_error_t pbdrv_led_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv);

#else // PBDRV_CONFIG_LED

static inline pbio_error_t pbdrv_led_get_dev(uint8_t id, pbdrv_led_dev_t **dev) {
    *dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_led_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif // PBDRV_CONFIG_LED

#endif // _PBDRV_LED_H_

/** @}*/
