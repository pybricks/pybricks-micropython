// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// LED array driver internal types and functions

#ifndef _PBDRV_LED_LED_ARRAY_H_
#define _PBDRV_LED_LED_ARRAY_H_

#include <stdio.h>

#include <pbdrv/config.h>
#include <pbdrv/led.h>
#include <pbio/color.h>
#include <pbio/error.h>

/**
 * Driver-specific callback functions.
 */
typedef struct {
    /**
     * Sets the brightness for a single LED in an array.
     * @param [in]  dev         The LED array device instance.
     * @param [in]  index       The index of the LED in the array.
     * @param [in]  brightness  The brightness (0 to 100).
     * @return                  ::PBIO_SUCCESS if successful.
     */
    pbio_error_t (*set_brightness)(pbdrv_led_array_dev_t *dev, uint8_t index, uint8_t brightness);
} pbdrv_led_array_funcs_t;

/** LED device instance. */
struct _pbdrv_led_array_dev_t {
    /** Platform-specific data */
    const void *pdata;
    /** Driver-specific callbacks. */
    const pbdrv_led_array_funcs_t *funcs;
};

#if PBDRV_CONFIG_LED_ARRAY

void pbdrv_led_array_init();

#else // PBDRV_CONFIG_LED_ARRAY

#define pbdrv_led_array_init()

#endif // PBDRV_CONFIG_LED_ARRAY

#endif // _PBDRV_LED_LED_ARRAY_H_
