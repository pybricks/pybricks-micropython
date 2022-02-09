// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/led.h>
#include <pbio/color.h>
#include <pbio/error.h>

#include "led_dual.h"
#include "led_pwm.h"
#include "led_virtual.h"
#include "led.h"

#ifndef PBDRV_CONFIG_LED_NUM_DEV
#error "Must define PBDRV_CONFIG_LED_NUM_DEV"
#endif

static pbdrv_led_dev_t pbdrv_led_dev[PBDRV_CONFIG_LED_NUM_DEV];

void pbdrv_led_init(void) {
    pbdrv_led_dual_init(pbdrv_led_dev);
    pbdrv_led_pwm_init(pbdrv_led_dev);
    pbdrv_led_virtual_init(pbdrv_led_dev);
}

/**
 * Gets an LED device instance.
 *
 * @param [in]  id      The ID of the device
 * @param [out] dev     Pointer to the device instance
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_NO_DEV if the ID was invalid,
 *                      ::PBIO_ERROR_AGAIN if the device is not yet initalized
 */
pbio_error_t pbdrv_led_get_dev(uint8_t id, pbdrv_led_dev_t **dev) {
    if (id >= PBDRV_CONFIG_LED_NUM_DEV) {
        return PBIO_ERROR_NO_DEV;
    }

    *dev = &pbdrv_led_dev[id];

    if ((*dev)->funcs == NULL) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}


/**
 * Sets the color of the LED to an HSV value.
 *
 * The V in HSV will be the brightness of the LED. Colors like brown and gray
 * are not possible since LEDs emit light rather than reflect it.
 *
 * The light may not be capable of displaying all colors or may not have
 * adjustable brightness. If a light is only white, the color values
 * will be averaged to give the final intensity. If the light only has one or
 * two of the possible three colors, the other color values will be ignored.
 * If the light is not capabile of adjusting the intensity, values < 50 will
 * be considered "off" and >= 50 will be considered as "on".
 * @param [in]  dev     The LED device instance
 * @param [in]  hsv     The color
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_led_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv) {
    return dev->funcs->set_hsv(dev, hsv);
}

#endif // PBDRV_CONFIG_LED
