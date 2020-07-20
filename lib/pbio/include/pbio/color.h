// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * \addtogroup Color Color Conversion Functions.
 *
 * Provides generic functions to manipulate colors.
 * @{
 */

#ifndef _PBIO_COLOR_H_
#define _PBIO_COLOR_H_

#include <stdint.h>

/**
 * Color names.
 */
typedef enum {
    PBIO_COLOR_NONE   = 0,   /**< no color */
    PBIO_COLOR_BLACK  = 1,   /**< black */
    PBIO_COLOR_BLUE   = 2,   /**< blue */
    PBIO_COLOR_GREEN  = 3,   /**< green */
    PBIO_COLOR_YELLOW = 4,   /**< yellow */
    PBIO_COLOR_RED    = 5,   /**< red */
    PBIO_COLOR_WHITE  = 6,   /**< white */
    PBIO_COLOR_BROWN  = 7,   /**< brown */
    PBIO_COLOR_ORANGE = 8,   /**< orange */
    PBIO_COLOR_PURPLE = 9,   /**< purple */
} pbio_color_t;

/** 24-bit RGB color. */
typedef struct {
    /** The red component. 0 to 255. */
    uint8_t r;
    /** The green component. 0 to 255. */
    uint8_t g;
    /** The blue component. 0 to 255. */
    uint8_t b;
} pbio_color_rgb_t;

/** HSV color. */
typedef struct {
    /** The hue component. 0 to 359 degrees. */
    uint16_t h;
    /** The saturation component. 0 to 100 percent. */
    uint8_t s;
    /** The value component. 0 to 100 percent. */
    uint8_t v;
} pbio_color_hsv_t;


void pbio_color_rgb_to_hsv(const pbio_color_rgb_t *rgb, pbio_color_hsv_t *hsv);

#endif // _PBIO_COLOR_H_

/**
 * @}
 */
