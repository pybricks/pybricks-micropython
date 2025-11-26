// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <assert.h>
#include <pbio/color.h>
#include <pbio/int_math.h>

/**
 * Gets squared Euclidean distance between HSV colors mapped into a
 * chroma-lightness-bicone. The bicone is 20000 units tall and 20000 units in
 * diameter.
 *
 * @param [in]  hsv_a    The first HSV color.
 * @param [in]  hsv_b    The second HSV color.
 * @returns              Squared distance (0 to 400000000).
 */
int32_t pbio_color_get_distance_bicone_squared(const pbio_color_hsv_t *hsv_a, const pbio_color_hsv_t *hsv_b) {

    // Chroma (= radial coordinate in bicone) of a and b (0-10000).
    int32_t radius_a = pbio_color_hsv_get_v(hsv_a) * hsv_a->s;
    int32_t radius_b = pbio_color_hsv_get_v(hsv_b) * hsv_b->s;

    // Lightness (= z-coordinate in bicone) of a and b (0-20000).
    // v is allowed to be negative, resulting in negative lightness.
    // This can be used to create a higher contrast between "none-color" and
    // normal colors.
    int32_t lightness_a = (200 - hsv_a->s) * hsv_a->v;
    int32_t lightness_b = (200 - hsv_b->s) * hsv_b->v;

    // z delta of a and b in HSV bicone (-20000, 20000).
    int32_t delta_z = (lightness_b - lightness_a);

    // x and y deltas of a and b in HSV bicone (-20000, 20000)
    int32_t delta_x = (radius_b * pbio_int_math_cos_deg(hsv_b->h) - radius_a * pbio_int_math_cos_deg(hsv_a->h)) / 10000;
    int32_t delta_y = (radius_b * pbio_int_math_sin_deg(hsv_b->h) - radius_a * pbio_int_math_sin_deg(hsv_a->h)) / 10000;

    // Squared Euclidean distance (0, 400000000)
    return delta_x * delta_x + delta_y * delta_y + delta_z * delta_z;
}

/**
 * Gets distance measure between a HSV color (a) and a fully or zero saturated
 * candidate color.
 *
 * @param [in]  measurement    The measured HSV color.
 * @param [in]  candidate      The candidate HSV color (an idealized color or grayscale).
 * @returns                    Heuristic distance.
 */
int32_t pbio_color_get_distance_saturation_heuristic(const pbio_color_hsv_t *measurement, const pbio_color_hsv_t *candidate) {

    bool idealized_grayscale = candidate->s == 0 && candidate->h == 0;
    bool idealized_color = candidate->s == 100 && candidate->v == 100;

    // Calling code needs to ensure this.
    assert(idealized_grayscale || idealized_color);

    uint32_t hue_dist = pbio_int_math_abs(candidate->h - measurement->h);
    if (hue_dist > 180) {
        hue_dist = 360 - hue_dist;
    }

    uint32_t value_dist = pbio_int_math_abs(candidate->v - measurement->v);

    const uint32_t penalty = 1000;

    if (measurement->s <= 40 || measurement->v <= 1) {
        // Measurement is unsaturated, so match to nearest grayscale; penalize color.
        if (idealized_grayscale) {
            // Match to nearest value.
            return value_dist;
        }
        // Looking for grayscale, so disqualify color candidate.
        return penalty + hue_dist;
    } else {
        // Measurement is saturated, so match to nearest full color; penalize grayscale.
        if (idealized_color) {
            // Match to nearest hue.
            return hue_dist;
        }
        // Looking for color, so disqualify grayscale candidate.
        return penalty + value_dist;
    }
}
