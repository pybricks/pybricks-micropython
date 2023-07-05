// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

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
int32_t pbio_color_get_bicone_squared_distance(const pbio_color_hsv_t *hsv_a, const pbio_color_hsv_t *hsv_b) {

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
