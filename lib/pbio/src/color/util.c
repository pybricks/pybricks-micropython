// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include <fixmath.h>

#include <pbio/color.h>

// Cost function between two colors a and b. The lower, the closer they are.
int32_t pbio_get_hsv_cost(const pbio_color_hsv_t *x, const pbio_color_hsv_t *c) {

    // Calculate the hue error
    int32_t hue_error;

    if (c->s <= 5 || x->s <= 5) {
        // When comparing against unsaturated colors,
        // the hue error is not so relevant.
        hue_error = 0;
    } else {
        hue_error = c->h > x->h ? c->h - x->h : x->h - c->h;
        if (hue_error > 180) {
            hue_error = 360 - hue_error;
        }
    }

    // Calculate the value error:
    int32_t value_error = x->v > c->v ? x->v - c->v : c->v - x->v;

    // Calculate the saturation error, with extra penalty for low saturation
    int32_t saturation_error = x->s > c->s ? x->s - c->s : c->s - x->s;
    saturation_error += (100 - c->s) / 2;

    // Total error
    return hue_error * hue_error + 5 * saturation_error * saturation_error + 2 * value_error * value_error;
}

// gets squared cartesian distance between hsv colors mapped into a chroma-value-cone
int32_t pbio_get_cone_cost(const pbio_color_hsv_t *hsv_a, const pbio_color_hsv_t *hsv_b) {

    pbio_color_hsv_fix16_t a, b;
    pbio_color_hsv_to_fix16(hsv_a, &a);
    pbio_color_hsv_to_fix16(hsv_b, &b);

    // x, y and z deltas between cartesian coordinates of a and b in HSV cone
    // delx = b_s*b_v*cos(b_h) - a_s*a_v*cos(a_h)
    fix16_t delx = fix16_sub(
        fix16_mul(fix16_mul(b.v, b.s), fix16_cos(b.h)),
        fix16_mul(fix16_mul(a.v, a.s), fix16_cos(a.h)));

    // dely = b_s*b_v*sin(b_h) - a_s*a_v*sin(a_h)
    fix16_t dely = fix16_sub(
        fix16_mul(fix16_mul(b.v, b.s), fix16_sin(b.h)),
        fix16_mul(fix16_mul(a.v, a.s), fix16_sin(a.h)));

    // delz = cone_height * (b_v - a_v)
    fix16_t delz = fix16_mul(F16C(1,000), fix16_sub(b.v, a.v));

    // cdist = delx*delx + dely*dely + delz*delz
    fix16_t cdist = fix16_add(
        fix16_add(
            fix16_sq(delx),
            fix16_sq(dely)),
        fix16_sq(delz));

    // multiply by large number to increase resolution when converting to int
    return fix16_to_int(fix16_mul(cdist, F16C(5000, 0000)));
}
