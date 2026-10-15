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
int32_t pbio_color_get_distance_bicone_squared(pbio_color_t hsv_a, pbio_color_t hsv_b) {

    // Chroma (= radial coordinate in bicone) of a and b (0-10000).
    int32_t radius_a = pbio_color_get_v_clamped(hsv_a) * pbio_color_get_s(hsv_a);
    int32_t radius_b = pbio_color_get_v_clamped(hsv_b) * pbio_color_get_s(hsv_b);

    // Lightness (= z-coordinate in bicone) of a and b (0-20000).
    // v is allowed to be negative, resulting in negative lightness.
    // This can be used to create a higher contrast between "none-color" and
    // normal colors.
    int32_t lightness_a = (200 - pbio_color_get_s(hsv_a)) * pbio_color_get_v(hsv_a);
    int32_t lightness_b = (200 - pbio_color_get_s(hsv_b)) * pbio_color_get_v(hsv_b);

    // z delta of a and b in HSV bicone (-20000, 20000).
    int32_t delta_z = (lightness_b - lightness_a);

    // x and y deltas of a and b in HSV bicone (-20000, 20000)
    int32_t delta_x = (radius_b * pbio_int_math_cos_deg(pbio_color_get_h(hsv_b)) - radius_a * pbio_int_math_cos_deg(pbio_color_get_h(hsv_a))) / 10000;
    int32_t delta_y = (radius_b * pbio_int_math_sin_deg(pbio_color_get_h(hsv_b)) - radius_a * pbio_int_math_sin_deg(pbio_color_get_h(hsv_a))) / 10000;

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
int32_t pbio_color_get_distance_saturation_heuristic(pbio_color_t measurement, pbio_color_t candidate) {

    bool idealized_grayscale = pbio_color_get_s(candidate) == 0 && pbio_color_get_h(candidate) == 0;
    bool idealized_color = pbio_color_get_s(candidate) == 100 && pbio_color_get_v(candidate) == 100;

    // Calling code needs to ensure this.
    assert(idealized_grayscale || idealized_color);

    uint32_t hue_dist = pbio_int_math_abs(pbio_color_get_h(candidate) - pbio_color_get_h(measurement));
    if (hue_dist > 180) {
        hue_dist = 360 - hue_dist;
    }

    uint32_t value_dist = pbio_int_math_abs(pbio_color_get_v(candidate) - pbio_color_get_v(measurement));

    const uint32_t penalty = 1000;

    if (pbio_color_get_s(measurement) <= 40 || pbio_color_get_v(measurement) <= 1) {
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
