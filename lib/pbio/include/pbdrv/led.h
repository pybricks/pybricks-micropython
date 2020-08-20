// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup LEDDriver LED I/O driver
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

/** @cond INTERNAL */
void pbdrv_led_init();
/** @endcond */

pbio_error_t pbdrv_led_get_dev(uint8_t id, pbdrv_led_dev_t **dev);
pbio_error_t pbdrv_led_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv);
pbio_error_t pbdrv_led_on(pbdrv_led_dev_t *dev, pbio_color_t color);
pbio_error_t pbdrv_led_off(pbdrv_led_dev_t *dev);

#else

#define pbdrv_led_init()

static inline pbio_error_t pbdrv_led_get_dev(uint8_t id, pbdrv_led_dev_t **dev) {
    *dev = NULL;
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_led_set_hsv(pbdrv_led_dev_t *dev, const pbio_color_hsv_t *hsv) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_led_on(pbdrv_led_dev_t *dev, pbio_color_t color) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbdrv_led_off(pbdrv_led_dev_t *dev) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

#endif

#endif // _PBDRV_LED_H_

/** @}*/
