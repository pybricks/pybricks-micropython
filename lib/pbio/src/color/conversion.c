// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

#include <pbio/color.h>

/**
 * Gets the the largest component of an RGB value.
 */
static uint8_t max_rgb(const pbio_color_rgb_t *rgb) {
    uint8_t ret = rgb->r;

    if (rgb->g > ret) {
        ret = rgb->g;
    }

    if (rgb->b > ret) {
        ret = rgb->b;
    }

    return ret;
}

/**
 * Gets the the smallest component of an RGB value.
 */
static uint8_t min_rgb(const pbio_color_rgb_t *rgb) {
    uint8_t ret = rgb->r;

    if (rgb->g < ret) {
        ret = rgb->g;
    }

    if (rgb->b < ret) {
        ret = rgb->b;
    }

    return ret;
}

/**
 * Converts RGB to HSV color value.
 *
 * Using basic method given by Wikipedia. https://en.wikipedia.org/wiki/HSL_and_HSV#From_RGB
 *
 * @param [in]  rgb         The source RGB color value.
 * @param [out] hsv         The destination HSV color value.
 */
void pbio_color_rgb_to_hsv(const pbio_color_rgb_t *rgb, pbio_color_hsv_t *hsv) {
    uint8_t max = max_rgb(rgb);
    uint8_t min = min_rgb(rgb);
    uint8_t chroma = max - min;

    hsv->h = 0;
    hsv->s = 0;

    if (chroma > 0) {
        uint8_t a, b, c;
        if (max == rgb->r) {
            a = rgb->g;
            b = rgb->b;
            c = 0;
        } else if (max == rgb->g) {
            a = rgb->b;
            b = rgb->r;
            c = 120;
        } else {
            a = rgb->r;
            b = rgb->g;
            c = 240;
        }
        int h = 60 * (a - b) / chroma + c;
        if (h < 0) {
            h += 360;
        }
        hsv->h = h;
        hsv->s = 100 * chroma / max;
    }

    // Multiplying by 101 and dividing by 256 is nearly the same as multiplying
    // by 100 and dividing by 255 but results in smaller binary code size.
    hsv->v = 101 * max / 256;
}

/**
 * Converts color name to HSV color value.
 *
 * @param [in]  color       The the source color.
 * @param [out] hsv         The destination HSV color value.
 */
void pbio_color_to_hsv(pbio_color_t color, pbio_color_hsv_t *hsv) {
    // See PBIO_COLOR_ENCODE in color.h for encoding scheme
    hsv->h = (color >> 3) * 30;
    hsv->s = (color & (0x1 << 2)) ? 100 : 0;
    hsv->v = (color & 0x2) ? 100 : ((color & 0x1) ? 50 : 0);
}
