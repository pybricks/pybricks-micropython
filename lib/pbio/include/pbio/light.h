// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

/**
 * @addtogroup Light Light control functions
 * @{
 */

#ifndef _PBIO_LIGHT_H_
#define _PBIO_LIGHT_H_

#include <pbio/color.h>
#include <pbio/config.h>
#include <pbio/error.h>

/** Color light instance. */
typedef struct _pbio_color_light_t pbio_color_light_t;

/** Sentinel value for a color light blink array. */
#define PBIO_COLOR_LIGHT_BLINK_END 0

/**
 * Convience macro for defining ::pbio_color_hsv_t animation cells.
 * @param [in]  hue         The hue (0 to 359)
 * @param [in]  saturation  The saturation (0 to 100)
 * @param [in]  value       The brightness (0 to 100)
 */
#define PBIO_COLOR_LIGHT_ANIMATION_CELL(hue, saturation, value) \
    { .h = (hue), .s = (saturation), .v = (value) }

/** Sentinel value for a color light animation array. */
#define PBIO_COLOR_LIGHT_ANIMATION_END { .h = UINT16_MAX }

#if PBIO_CONFIG_LIGHT

pbio_error_t pbio_color_light_on_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv);
pbio_error_t pbio_color_light_on(pbio_color_light_t *light, pbio_color_t color);
pbio_error_t pbio_color_light_off(pbio_color_light_t *light);
void pbio_color_light_start_blink_animation(pbio_color_light_t *light, const pbio_color_hsv_t *hsv, const uint16_t *cells);
void pbio_color_light_start_animation(pbio_color_light_t *light, uint16_t interval, const pbio_color_hsv_t *cells);

#else // PBIO_CONFIG_LIGHT

static inline pbio_error_t pbio_color_light_on_hsv(pbio_color_light_t *light, const pbio_color_hsv_t *hsv) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_color_light_on(pbio_color_light_t *light, pbio_color_t color) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline pbio_error_t pbio_color_light_off(pbio_color_light_t *light) {
    return PBIO_ERROR_NOT_SUPPORTED;
}

static inline void pbio_color_light_start_blink_animation(pbio_color_light_t *light, const pbio_color_hsv_t *hsv, const uint16_t *cells) {
}

static inline void pbio_color_light_start_animation(pbio_color_light_t *light, uint16_t interval, const pbio_color_hsv_t *cells) {
}

#endif // PBIO_CONFIG_LIGHT

#endif // _PBIO_LIGHT_H_

/** @} */
