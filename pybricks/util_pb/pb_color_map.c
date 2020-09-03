// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdint.h>
#include <stdio.h>

#include <pbio/error.h>

#include "py/obj.h"

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_error.h>

STATIC const mp_rom_obj_tuple_t pb_color_map_default = {
    {&mp_type_tuple},
    7,
    {
        MP_OBJ_FROM_PTR(&pb_Color_RED_obj),
        MP_OBJ_FROM_PTR(&pb_Color_YELLOW_obj),
        MP_OBJ_FROM_PTR(&pb_Color_GREEN_obj),
        MP_OBJ_FROM_PTR(&pb_Color_BLUE_obj),
        MP_OBJ_FROM_PTR(&pb_Color_BLACK_obj),
        MP_OBJ_FROM_PTR(&pb_Color_WHITE_obj),
        mp_const_none,
    }
};

// Set initial default map
void pb_color_map_save_default(mp_obj_t *color_map) {
    *color_map = MP_OBJ_FROM_PTR(&pb_color_map_default);
}

// Cost function between two colors a and b. The lower, the closer they are.
static int32_t get_hsv_cost(const pbio_color_hsv_t *x, const pbio_color_hsv_t *c) {

    // TODO: Actual 3D cost. For now, use the simplified threshold-based
    // mapping we used to have.

    // For low saturation, pick the closest value among unsaturated colors
    if (x->s < 40 || x->v <= 5) {
        // Ignore comparison to saturated colors
        if (c->s == 100) {
            return INT32_MAX;
        }
        // Return absolute value error
        return x->v > c->v ? x->v - c->v : c->v - x->v;
    }
    // Otherwise, return the hue error
    int32_t hue_error = c->h > x->h ? c->h - x->h : x->h - c->h;
    return hue_error <= 180 ? hue_error : 360 - hue_error;
}

// Get a discrete color that matches the given hsv values most closely
mp_obj_t pb_color_map_get_color(mp_obj_t *color_map, pbio_color_hsv_t *hsv) {

    // Unpack the main list
    mp_obj_t *colors;
    size_t n;
    mp_obj_get_array(*color_map, &n, &colors);

    // Initialize minimal cost to maximum
    mp_obj_t match = mp_const_none;
    int32_t cost_now = INT32_MAX;
    int32_t cost_min = INT32_MAX;

    // Compute cost for each candidate
    for (size_t i = 0; i < n; i++) {

        // Get HSV of the candidate discrete color
        const pbio_color_hsv_t *compare;
        if (colors[i] == mp_const_none) {
            compare = &pb_Color_NONE_obj.hsv;
        } else {
            pb_type_Color_obj_t *color = MP_OBJ_TO_PTR(colors[i]);
            compare = &color->hsv;
        }

        // Evaluate the cost function
        cost_now = get_hsv_cost(hsv, compare);

        // If cost is less than before, update the minimum and the match
        if (cost_now < cost_min) {
            cost_min = cost_now;
            match = colors[i];
        }
    }
    return match;
}

// Generic class structure for ColorDistanceSensor
// Any color sensor structure with a color_map
// must have base and color_map as the first two members.
typedef struct _pb_ColorSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t color_map;
} pb_ColorSensor_obj_t;

// pybricks._common.ColorDistanceSensor.color_map
STATIC mp_obj_t pupdevices_ColorDistanceSensor_color_map(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_NONE(colors));

    // If no arguments are given, return current map
    if (colors_in == mp_const_none) {
        return self->color_map;
    }

    // Otherwise, verify and save given map TODO: verification
    self->color_map = colors_in;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pb_ColorSensor_color_map_obj, 1, pupdevices_ColorDistanceSensor_color_map);
