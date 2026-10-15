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
#include <pbio/os.h>
#include <pbsys/telemetry.h>

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
 * fixed-size chunks so that the packet location maps to a fixed frame
 * position. The driver tracks its own read-out progress. Frames may be torn
 * by concurrent display updates, in which case a newer frame follows to fix it.
 *
 * @param [out] tel   Packet to populate with one chunk.
 * @param [out] done  Set when the caller should stop requesting chunks, which
 *                    is after the last chunk of a frame or when there is
 *                    nothing to send. Left alone when out of room, so that the
 *                    caller retries the same chunk with a fresh buffer.
 * @param [inout] size  Bytes available for the payload on entry, bytes
 *                      written on return.
 * @return  ::PBSYS_TELEMETRY_SUCCESS if a chunk was written,
 *          ::PBSYS_TELEMETRY_ERROR_NO_ROOM if it does not fit in @p size, or
 *          ::PBSYS_TELEMETRY_ERROR_NO_REPORT if there is nothing new to send.
 */
pbsys_telemetry_error_t pbdrv_display_iterate_data(pbsys_telemetry_packet_t *tel, bool *done, uint32_t *size);

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

static inline pbsys_telemetry_error_t pbdrv_display_iterate_data(pbsys_telemetry_packet_t *tel, bool *done, uint32_t *size) {
    *done = true;
    *size = 0;
    return PBSYS_TELEMETRY_ERROR_NO_REPORT;
}

#endif // PBDRV_CONFIG_DISPLAY

#endif // _PBDRV_DISPLAY_H_

/** @} */
