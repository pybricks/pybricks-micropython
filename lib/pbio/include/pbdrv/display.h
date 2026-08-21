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
 * Get the pixel value corresponding to a color given in HSV space.
 * @param [in] h  Hue, 0 to 359 degrees.
 * @param [in] s  Saturation, 0 to 100 percent.
 * @param [in] v  Value, 0 to 100 percent.
 * @return  Pixel value.
 */
uint8_t pbdrv_display_get_value_from_hsv(uint16_t h, uint8_t s, uint8_t v);

/**
 * Update the display to show current content of image container.
 */
void pbdrv_display_update(void);

/**
 * Maximum number of bytes returned per call to
 * pbdrv_display_get_telemetry_data().
 */
#define PBDRV_DISPLAY_TELEMETRY_MAX_SIZE (500)

/**
 * Gets the next chunk of display telemetry data, if any.
 *
 * Data is in a device-specific encoding such as packed pixels, produced in
 * fixed-size chunks so that @p location maps to a fixed frame position. The
 * driver tracks its own read-out progress, bounded to one frame at a time.
 * Frames may be torn by concurrent display updates, in which case a newer
 * frame follows to fix it.
 *
 * @param [out] buffer    Buffer to copy into. Must fit at least
 *                        ::PBDRV_DISPLAY_TELEMETRY_MAX_SIZE bytes.
 * @param [out] location  Chunk index of the returned data within the frame.
 * @return  Number of bytes copied. Zero if there is nothing new to send or
 *          if the driver does not provide telemetry data.
 */
uint32_t pbdrv_display_get_telemetry_data(uint8_t *buffer, uint32_t *location);

#else // PBDRV_CONFIG_DISPLAY

static inline pbio_image_t *pbdrv_display_get_image(void) {
    return NULL;
}

static inline uint8_t pbdrv_display_get_max_value(void) {
    return 0;
}

static inline uint8_t pbdrv_display_get_value_from_hsv(uint16_t h, uint8_t s, uint8_t v) {
    return 0;
}

static inline void pbdrv_display_update(void) {
}

static inline uint32_t pbdrv_display_get_telemetry_data(uint8_t *buffer, uint32_t *location) {
    *location = 0;
    return 0;
}

#endif // PBDRV_CONFIG_DISPLAY

#endif // _PBDRV_DISPLAY_H_

/** @} */
