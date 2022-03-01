// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <fixmath.h>

#include <pbio/color.h>

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

int32_t pbio_get_cone_cost(const pbio_color_hsv_t *a, const pbio_color_hsv_t *b) {
    // normalize h to radians, s/v to (0,1)
    fix16_t by100 = fix16_div(fix16_one, fix16_from_int(100));
    fix16_t a_h = fix16_deg_to_rad(fix16_from_int(a->h));
    fix16_t b_h = fix16_deg_to_rad(fix16_from_int(b->h));
    fix16_t a_s = fix16_mul(fix16_from_int(a->s), by100);
    fix16_t b_s = fix16_mul(fix16_from_int(b->s), by100);
    fix16_t a_v = fix16_mul(fix16_from_int(a->v), by100);
    fix16_t b_v = fix16_mul(fix16_from_int(b->v), by100);

    // x, y and z deltas between cartesian coordinates of a and b in HSV cone
    // delx = b_s*b_v*cos(b_h) - a_s*a_v*cos(a_h)
    fix16_t delx = fix16_sub(
        fix16_mul(
            fix16_mul(
                fix16_cos(b_h),
                b_v),
            b_s),
        fix16_mul(
            fix16_mul(
                fix16_cos(a_h),
                a_v),
            a_s));

    // dely = b_s*b_v*sin(b_h) - a_s*a_v*sin(a_h)
    fix16_t dely = fix16_sub(
        fix16_mul(
            fix16_mul(
                fix16_sin(b_h),
                b_v),
            b_s),
        fix16_mul(
            fix16_mul(
                fix16_sin(a_h),
                a_v),
            a_s));
    // delz = b_v - a_v
    fix16_t delz = fix16_sub(b_v, a_v);

    // cdist = delx*delx + dely*dely + delz*delz
    fix16_t cdist = fix16_add(
        fix16_add(
            fix16_sq(delx),
            fix16_sq(dely)),
        fix16_sq(delz));
    return fix16_to_int(cdist);
}
