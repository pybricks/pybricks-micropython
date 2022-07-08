// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <pbdrv/config.h>

#if PBDRV_CONFIG_LED_ARRAY

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/led.h>
#include <pbio/error.h>

#include "led_array_pwm.h"
#include "led_array.h"

#ifndef PBDRV_CONFIG_LED_ARRAY_NUM_DEV
#error "Must define PBDRV_CONFIG_LED_ARRAY_NUM_DEV"
#endif

static pbdrv_led_array_dev_t pbdrv_led_array_dev[PBDRV_CONFIG_LED_ARRAY_NUM_DEV];

void pbdrv_led_array_init(void) {
    pbdrv_led_array_pwm_init(pbdrv_led_array_dev);
}

/**
 * Gets an LED array device instance.
 *
 * @param [in]  id      The ID of the device
 * @param [out] dev     Pointer to the device instance
 * @return              ::PBIO_SUCCESS if the call was successful,
 *                      ::PBIO_ERROR_NO_DEV if the ID was invalid,
 *                      ::PBIO_ERROR_AGAIN if the device is not yet initialized
 */
pbio_error_t pbdrv_led_array_get_dev(uint8_t id, pbdrv_led_array_dev_t **dev) {
    if (id >= PBDRV_CONFIG_LED_ARRAY_NUM_DEV) {
        return PBIO_ERROR_NO_DEV;
    }

    *dev = &pbdrv_led_array_dev[id];

    if ((*dev)->funcs == NULL) {
        // has not been initialized yet
        return PBIO_ERROR_AGAIN;
    }

    return PBIO_SUCCESS;
}

/**
 * Sets the brightness of the LED array.
 *
 * Sets the brightness for a single LED in an array.
 * @param [in]  dev         The LED array device instance.
 * @param [in]  index       The index of the LED in the array.
 * @param [in]  brightness  The brightness (0 to 100).
 * @return                  ::PBIO_SUCCESS if the call was successful,
 *                          ::PBIO_ERROR_IO if there was an I/O error
 */
pbio_error_t pbdrv_led_array_set_brightness(pbdrv_led_array_dev_t *dev, uint8_t index, uint8_t brightness) {
    return dev->funcs->set_brightness(dev, index, brightness);
}

#endif // PBDRV_CONFIG_LED_ARRAY
