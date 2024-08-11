// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_DEVICES

#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>


#include <stdint.h>
#include <stdio.h>

#include <pbio/error.h>
#include <pbio/color.h>

#include "py/obj.h"

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_error.h>

// This expands pbio_color_rgb_to_hsv with additional calibration steps that
// ultimately must be properly done in pbio_color_rgb_to_hsv, just like
// pbio_color_hsv_to_rgb, by adjusting RGB instead of hacking at the HSV value.
void pb_color_map_rgb_to_hsv(const pbio_color_rgb_t *rgb, pbio_color_hsv_t *hsv) {

    // Standard conversion
    pbio_color_rgb_to_hsv(rgb, hsv);

    // Slight shift for lower hues to make yellow somewhat more accurate
    if (hsv->h < 40) {
        uint8_t offset = ((hsv->h - 20) << 8) / 20;
        int32_t scale = 200 - ((100 * (offset * offset)) >> 16);
        hsv->h = hsv->h * scale / 100;
    }

    // Value and saturation correction
    hsv->s = hsv->s * (200 - hsv->s) / 100;
    hsv->v = hsv->v * (200 - hsv->v) / 100;
}

static const mp_rom_obj_tuple_t pb_color_map_default = {
    {&mp_type_tuple},
    6,
    {
        MP_OBJ_FROM_PTR(&pb_Color_RED_obj),
        MP_OBJ_FROM_PTR(&pb_Color_YELLOW_obj),
        MP_OBJ_FROM_PTR(&pb_Color_GREEN_obj),
        MP_OBJ_FROM_PTR(&pb_Color_BLUE_obj),
        MP_OBJ_FROM_PTR(&pb_Color_WHITE_obj),
        MP_OBJ_FROM_PTR(&pb_Color_NONE_obj),
    }
};

// Set initial default map
void pb_color_map_save_default(mp_obj_t *color_map) {
    *color_map = MP_OBJ_FROM_PTR(&pb_color_map_default);
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

        // Evaluate the cost function
        cost_now = pbio_color_get_bicone_squared_distance(hsv, pb_type_Color_get_hsv(colors[i]));

        // If cost is less than before, update the minimum and the match
        if (cost_now < cost_min) {
            cost_min = cost_now;
            match = colors[i];
        }
    }
    return match;
}

// HACK: all color sensor structures must have color_map as second item
// REVISIT: Replace with a safer solution to share this method across sensors
typedef struct _pb_ColorSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    mp_obj_t color_map;
} pb_ColorSensor_obj_t;

// pybricks._common.ColorDistanceSensor.detectable_colors
static mp_obj_t pupdevices_ColorDistanceSensor_detectable_colors(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_NONE(colors));

    // If no arguments are given, return current map
    if (colors_in == mp_const_none) {
        return self->color_map;
    }

    // If arguments given, ensure all tuple elements have the right type
    mp_obj_t *color_objs;
    size_t n;
    mp_obj_get_array(colors_in, &n, &color_objs);
    for (size_t i = 0; i < n; i++) {
        pb_assert_type(color_objs[i], &pb_type_Color);
    }

    // Save the given map
    self->color_map = colors_in;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pb_ColorSensor_detectable_colors_obj, 1, pupdevices_ColorDistanceSensor_detectable_colors);

#endif // PYBRICKS_PY_NXTDEVICES || PYBRICKS_PY_PUPDEVICES
