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
 * Copies a chunk of the display frame buffer, in a device-specific encoding
 * such as packed pixels. Offset and sizes are in encoded bytes.
 *
 * The update count is incremented on every display update. Since the frame
 * is typically read in chunks across multiple calls, the caller should keep
 * reading until it has read one full frame with an unchanged update count,
 * starting over from offset 0 when the count changes mid-frame.
 *
 * @param [out] buffer        Buffer to copy into.
 * @param [in]  offset        Offset into the frame buffer to start reading.
 * @param [in]  max_size      How much can be read at most.
 * @param [out] update_count  Number of display updates so far.
 * @return  Number of bytes copied. Zero if @p offset is at or beyond the end
 *          of the frame, or if the driver does not provide a buffer.
 */
uint32_t pbdrv_display_get_buffer(uint8_t *buffer, uint32_t offset, uint32_t max_size, uint32_t *update_count);

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

static inline uint32_t pbdrv_display_get_buffer(uint8_t *buffer, uint32_t offset, uint32_t max_size, uint32_t *update_count) {
    *update_count = 0;
    return 0;
}

#endif // PBDRV_CONFIG_DISPLAY

#endif // _PBDRV_DISPLAY_H_

/** @} */
