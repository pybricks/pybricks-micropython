// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

/**
 * @addtogroup DisplayDriver Driver: Display
 * @{
 */

#ifndef _PBDRV_DISPLAY_H_
#define _PBDRV_DISPLAY_H_

#include <pbdrv/config.h>
#include <pbio/image.h>

#if PBDRV_CONFIG_DISPLAY

/**
 * Get an image container representing the display.
 * @return  Image container, or NULL if no display.
 */
pbio_image_t *pbdrv_display_get_image(void);

/**
 * Get the maximum value of a pixel.
 * @return  Maximum value, corresponding to black on a LCD screen.
 */
uint8_t pbdrv_display_get_max_value(void);

/**
 * Update the display to show current content of image container.
 */
void pbdrv_display_update(void);

#else // PBDRV_CONFIG_DISPLAY

static inline pbio_image_t *pbdrv_display_get_image(void) {
    return NULL;
}

static inline uint8_t pbdrv_display_get_max_value(void) {
    return 0;
}

static inline void pbdrv_display_update(void) {
}

#endif // PBDRV_CONFIG_DISPLAY

#endif // _PBDRV_DISPLAY_H_

/** @} */
