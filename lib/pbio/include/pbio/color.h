// SPDX-License-Identifier: MIT
// Copyright (c) 2020,2022 The Pybricks Authors

/**
 * @addtogroup Color pbio/color: Color Conversion Functions
 *
 * Provides generic functions to manipulate colors.
 * @{
 */

#ifndef _PBIO_COLOR_H_
#define _PBIO_COLOR_H_

#include <stdint.h>

/** @cond INTERNAL */

/**
 * Packs HSV into 32 bits as 0xHHHHSSVV.
 *
 * @param h [in]    The hue, 0..359 degrees
 * @param s [in]    The saturation, 0..100 percent
 * @param v [in]    The value, 0..100 percent, or negative (see ::pbio_color_get_v)
 */
#define PBIO_COLOR_ENCODE(h, s, v) (((h) << 16) | ((s) << 8) | ((v) & 0xff))

/** @endcond */

/**
 * HSV color, packed as 0xHHHHSSVV. Named values are the standard colors.
 */
typedef enum {
    PBIO_COLOR_NONE = PBIO_COLOR_ENCODE(0, 0, 0), /**< no color, i.e. no light at all */
    PBIO_COLOR_BLACK = PBIO_COLOR_ENCODE(0, 0, 10), /**< black, which still reflects some light */
    PBIO_COLOR_GRAY = PBIO_COLOR_ENCODE(0, 0, 50), /**< gray */
    PBIO_COLOR_WHITE = PBIO_COLOR_ENCODE(0, 0, 100), /**< white */
    PBIO_COLOR_RED = PBIO_COLOR_ENCODE(0, 100, 100), /**< red */
    PBIO_COLOR_BROWN = PBIO_COLOR_ENCODE(30, 100, 50), /**< brown */
    PBIO_COLOR_ORANGE = PBIO_COLOR_ENCODE(30, 100, 100), /**< orange */
    PBIO_COLOR_YELLOW = PBIO_COLOR_ENCODE(60, 100, 100), /**< yellow */
    PBIO_COLOR_GREEN = PBIO_COLOR_ENCODE(120, 100, 100), /**< green */
    PBIO_COLOR_SPRING_GREEN = PBIO_COLOR_ENCODE(150, 100, 100), /**< spring green */
    PBIO_COLOR_CYAN = PBIO_COLOR_ENCODE(180, 100, 100), /**< cyan */
    PBIO_COLOR_BLUE = PBIO_COLOR_ENCODE(240, 100, 100), /**< blue */
    PBIO_COLOR_VIOLET = PBIO_COLOR_ENCODE(270, 100, 100), /**< violet */
    PBIO_COLOR_MAGENTA = PBIO_COLOR_ENCODE(300, 100, 100), /**< magenta */
    // Hues of 360 and up cannot occur in real colors, so they are used for sentinels.
    /**
     * Not a color, but a sentinel for showing whatever is underneath, such as
     * a lower priority indication.
     */
    PBIO_COLOR_TRANSPARENT = PBIO_COLOR_ENCODE(360, 0, 0),
    /** Not a color, but a sentinel that marks the end of an array of colors. */
    PBIO_COLOR_ARRAY_END = PBIO_COLOR_ENCODE(361, 0, 0),
} pbio_color_t;

/** Color hues for HSV color space. Values are in degrees (0 to 359). */
typedef enum {
    /** Red. */
    PBIO_COLOR_HUE_RED = 0,
    /** Orange. */
    PBIO_COLOR_HUE_ORANGE = 30,
    /** Yellow. */
    PBIO_COLOR_HUE_YELLOW = 60,
    /** Green. */
    PBIO_COLOR_HUE_GREEN = 120,
    /** Spring green. */
    PBIO_COLOR_HUE_SPRING_GREEN = 150,
    /** Cyan. */
    PBIO_COLOR_HUE_CYAN = 180,
    /** Blue. */
    PBIO_COLOR_HUE_BLUE = 240,
    /** Violet. */
    PBIO_COLOR_HUE_VIOLET = 270,
    /** Magenta. */
    PBIO_COLOR_HUE_MAGENTA = 300,
} pbio_color_hue_t;

/** The modulo value to get colors into 0 to 359 value range. */
#define PBIO_COLOR_HUE_MODULO 360

/** 24-bit RGB color. */
typedef struct {
    /** The red component. 0 to 255. */
    uint8_t r;
    /** The green component. 0 to 255. */
    uint8_t g;
    /** The blue component. 0 to 255. */
    uint8_t b;
} pbio_color_rgb_t;

/** Gets the hue component, 0 to 359 degrees. */
static inline uint16_t pbio_color_get_h(pbio_color_t color) {
    return color >> 16;
}

/** Gets the saturation component, 0 to 100 percent. */
static inline uint8_t pbio_color_get_s(pbio_color_t color) {
    return (color >> 8) & 0xff;
}

/**
 * Gets the value component as stored. Normally 0 to 100 percent but allowed to
 * be negative to provide higher contrast in color scanning applications.
 *
 * Use ::pbio_color_get_v_clamped instead where a negative value makes no sense,
 * such as when driving a light.
 */
static inline int8_t pbio_color_get_v(pbio_color_t color) {
    return (int8_t)(color & 0xff);
}

/** Gets the value component like ::pbio_color_get_v, with negative values clamped to zero. */
static inline uint8_t pbio_color_get_v_clamped(pbio_color_t color) {
    int8_t v = pbio_color_get_v(color);
    return v < 0 ? 0 : v;
}

pbio_color_t pbio_color_from_rgb(const pbio_color_rgb_t *rgb);
void pbio_color_to_rgb(pbio_color_t color, pbio_color_rgb_t *rgb);

typedef int32_t (*pbio_color_distance_func_t)(pbio_color_t hsv_a, pbio_color_t hsv_b);

int32_t pbio_color_get_distance_bicone_squared(pbio_color_t hsv_a, pbio_color_t hsv_b);
int32_t pbio_color_get_distance_saturation_heuristic(pbio_color_t hsv_a, pbio_color_t hsv_b);

#endif // _PBIO_COLOR_H_

/** @} */
