// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors
// Copyright (c) 2013 FastLED

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

// The following code derived from hsv2rgb_raw_C() and hsv2rgb_spectrum() in the FastLED project
// https://github.com/FastLED/FastLED/blob/master/hsv2rgb.cpp

/**
 * Converts HSV to RGB color value.
 *
 * This method takes into account apparent brightness which works nicely with
 * things that emit light, like LEDs.
 *
 * This is not the direct inverse of pbio_color_rgb_to_hsv().
 *
 * @param [in]  hsv         The source HSV color value.
 * @param [out] rgb         The destination RGB color value.
 */
void pbio_color_hsv_to_rgb(const pbio_color_hsv_t *hsv, pbio_color_rgb_t *rgb) {
    // scale hue to a max value of 191
    uint8_t hue = 273 * hsv->h / 512;

    // Convert hue, saturation and brightness (HSV/HSB) to RGB
    // "Dimming" is used on saturation and brightness to make
    // the output more visually linear.

    // Scale 0..100 percent to 0..255
    uint8_t value = 327 * hsv->v / 128;
    uint8_t saturation = 327 * hsv->s / 128;

    // The brightness floor is minimum number that all of
    // R, G, and B will be set to.
    uint8_t invsat = 255 - saturation;
    uint8_t brightness_floor = (value * invsat) / 256;

    // The color amplitude is the maximum amount of R, G, and B
    // that will be added on top of the brightness_floor to
    // create the specific hue desired.
    uint8_t color_amplitude = value - brightness_floor;

    // Figure out which section of the hue wheel we're in,
    // and how far offset we are withing that section
    uint8_t section = hue / 64; // 0..2
    uint8_t offset = hue % 64;  // 0..63

    uint8_t rampup = offset; // 0..63
    uint8_t rampdown = 63 - offset; // 63..0

    // We now scale rampup and rampdown to a 0-255 range -- at least
    // in theory, but here's where architecture-specific decsions
    // come in to play:
    // To scale them up to 0-255, we'd want to multiply by 4.
    // But in the very next step, we multiply the ramps by other
    // values and then divide the resulting product by 256.
    // So which is faster?
    //   ((ramp * 4) * othervalue) / 256
    // or
    //   ((ramp) * othervalue) /  64
    // It depends on your processor architecture.
    // On 8-bit AVR, the "/ 256" is just a one-cycle register move,
    // but the "/ 64" might be a multicycle shift process. So on AVR
    // it's faster do multiply the ramp values by four, and then
    // divide by 256.
    // On ARM, the "/ 256" and "/ 64" are one cycle each, so it's
    // faster to NOT multiply the ramp values by four, and just to
    // divide the resulting product by 64 (instead of 256).
    // Moral of the story: trust your profiler, not your instincts.

    // Since there's an AVR assembly version elsewhere, we'll
    // assume what we're on an architecture where any number of
    // bit shifts has roughly the same cost, and we'll remove the
    // redundant math at the source level:

    // // scale up to 255 range
    // rampup *= 4; // 0..252
    // rampdown *= 4; // 0..252

    // compute color-amplitude-scaled-down versions of rampup and rampdown
    uint8_t rampup_amp_adj = (rampup * color_amplitude) / (256 / 4);
    uint8_t rampdown_amp_adj = (rampdown * color_amplitude) / (256 / 4);

    // add brightness_floor offset to everything
    uint8_t rampup_adj_with_floor = rampup_amp_adj + brightness_floor;
    uint8_t rampdown_adj_with_floor = rampdown_amp_adj + brightness_floor;

    if (section) {
        if (section == 1) {
            // section 1: 0x40..0x7F
            rgb->r = brightness_floor;
            rgb->g = rampdown_adj_with_floor;
            rgb->b = rampup_adj_with_floor;
        } else {
            // section 2; 0x80..0xBF
            rgb->r = rampup_adj_with_floor;
            rgb->g = brightness_floor;
            rgb->b = rampdown_adj_with_floor;
        }
    } else {
        // section 0: 0x00..0x3F
        rgb->r = rampdown_adj_with_floor;
        rgb->g = rampup_adj_with_floor;
        rgb->b = brightness_floor;
    }
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

/**
 * Converts color name to RGB color value.
 *
 * See pbio_color_hsv_to_rgb() for more information.
 *
 * @param [in]  color       The the source color.
 * @param [out] rgb         The destination RGB color value.
 */
void pbio_color_to_rgb(pbio_color_t color, pbio_color_rgb_t *rgb) {
    pbio_color_hsv_t hsv;
    pbio_color_to_hsv(color, &hsv);
    pbio_color_hsv_to_rgb(&hsv, rgb);
}
