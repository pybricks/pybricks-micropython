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




// Hue or value not specified
const int32_t NA = -361;

// Set initial default thresholds
void pb_hsv_map_save_default(pb_hsv_map_t *map) {
    map->saturation_threshold = 30;
    map->hue_red = 350;
    map->hue_orange = NA;
    map->hue_yellow = 30;
    map->hue_green = 110;
    map->hue_cyan = NA;
    map->hue_blue = 210;
    map->hue_purple = NA;
    map->hue_magenta = NA;
    map->value_none = 0;
    map->value_black = 10;
    map->value_gray = NA;
    map->value_white = 60;
}

static void update_error(int32_t value, int32_t *min_error, mp_obj_t *color_match, int32_t compare, mp_obj_t color) {

    // Do not process N/A colors
    if (compare == NA) {
        return;
    }

    // Get error
    int32_t error = value - compare;
    error = error > 0 ? error : -error;
    error = error > 180 ? 360 - error : error;

    // If this is the new minimum, update
    if (error < *min_error) {
        *min_error = error;
        *color_match = color;
    }
}

// Get a discrete color that matches the given hsv values most closely
mp_obj_t pb_hsv_get_color(pb_hsv_map_t *map, int32_t hue, int32_t saturation, int32_t value) {

    int32_t min_error = 1000;
    mp_obj_t color_match = mp_const_none;

    if (saturation >= map->saturation_threshold) {
        // Pick a color based on hue, whichever is the nearest match
        update_error(hue, &min_error, &color_match, map->hue_red, pb_const_color_red);
        update_error(hue, &min_error, &color_match, map->hue_orange, pb_const_color_orange);
        update_error(hue, &min_error, &color_match, map->hue_yellow, pb_const_color_yellow);
        update_error(hue, &min_error, &color_match, map->hue_green, pb_const_color_green);
        update_error(hue, &min_error, &color_match, map->hue_cyan, pb_const_color_cyan);
        update_error(hue, &min_error, &color_match, map->hue_blue, pb_const_color_blue);
        update_error(hue, &min_error, &color_match, map->hue_purple, pb_const_color_purple);
        update_error(hue, &min_error, &color_match, map->hue_magenta, pb_const_color_magenta);
    } else {
        // Pick a non-color depending on value, whichever is the nearest match
        update_error(value, &min_error, &color_match, map->value_none, mp_const_none);
        update_error(value, &min_error, &color_match, map->value_black, pb_const_color_black);
        update_error(value, &min_error, &color_match, map->value_gray, pb_const_color_gray);
        update_error(value, &min_error, &color_match, map->value_white, pb_const_color_white);
    }

    return color_match;
}

// Return the color map as MicroPython objects
mp_obj_t pack_color_map(pb_hsv_map_t *map) {

    // Pack hue dictionary
    mp_obj_dict_t *hues = mp_obj_new_dict(0);
    if (map->hue_red != NA) {
        mp_obj_dict_store(hues, pb_const_color_red, mp_obj_new_int(map->hue_red));
    }
    if (map->hue_orange != NA) {
        mp_obj_dict_store(hues, pb_const_color_orange, mp_obj_new_int(map->hue_orange));
    }
    if (map->hue_yellow != NA) {
        mp_obj_dict_store(hues, pb_const_color_yellow, mp_obj_new_int(map->hue_yellow));
    }
    if (map->hue_green != NA) {
        mp_obj_dict_store(hues, pb_const_color_green, mp_obj_new_int(map->hue_green));
    }
    if (map->hue_blue != NA) {
        mp_obj_dict_store(hues, pb_const_color_blue, mp_obj_new_int(map->hue_blue));
    }
    if (map->hue_cyan != NA) {
        mp_obj_dict_store(hues, pb_const_color_cyan, mp_obj_new_int(map->hue_cyan));
    }
    if (map->hue_purple != NA) {
        mp_obj_dict_store(hues, pb_const_color_purple, mp_obj_new_int(map->hue_purple));
    }
    if (map->hue_magenta != NA) {
        mp_obj_dict_store(hues, pb_const_color_magenta, mp_obj_new_int(map->hue_magenta));
    }

    // Pack saturation threshold
    mp_obj_t saturation = mp_obj_new_int(map->saturation_threshold);

    // Pack value dictionary
    mp_obj_dict_t *values = mp_obj_new_dict(0);
    if (map->value_none != NA) {
        mp_obj_dict_store(values, mp_const_none, mp_obj_new_int(map->value_none));
    }
    if (map->value_black != NA) {
        mp_obj_dict_store(values, pb_const_color_black, mp_obj_new_int(map->value_black));
    }
    if (map->value_gray != NA) {
        mp_obj_dict_store(values, pb_const_color_gray, mp_obj_new_int(map->value_gray));
    }
    if (map->value_white != NA) {
        mp_obj_dict_store(values, pb_const_color_white, mp_obj_new_int(map->value_white));
    }

    mp_obj_t ret[3];
    ret[0] = hues;
    ret[1] = saturation;
    ret[2] = values;

    return mp_obj_new_tuple(3, ret);
}

// Get the hue/value from the dict if it contains that color key
static int32_t get_hue_or_value(mp_obj_t dict, mp_obj_t color) {
    mp_obj_dict_t *map = MP_OBJ_TO_PTR(dict);
    mp_map_elem_t *elem = mp_map_lookup(&map->map, color, MP_MAP_LOOKUP);
    if (elem == NULL) {
        return NA;
    }
    return pb_obj_get_int(elem->value);
}

// Ensure that the color hue or value is greater than previous.
// Also keep track of the minimum and maximum valid values.
static void assert_greater(int32_t value, int32_t *min, int32_t *max, int32_t abs_max) {
    // If this color is not specified, do nothing
    if (value == NA) {
        return;
    }
    // If no valid minimum is stored yet and we have a valid value, this is the minimum
    if (*min == NA || value < 0 || value > abs_max) {
        *min = value;
    }
    // If we have a value, make sure it is within the valid range and greater than current maximum
    if (value <= *max || value < 0 || value > abs_max) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    // Update the maximum
    *max = value;
}

// Unpack MicroPython color map data and verify integrity (monoticity of values)
void unpack_color_map(pb_hsv_map_t *map, mp_obj_t hues, mp_obj_t saturation, mp_obj_t values) {

    // Get user specified hues and ensure their values are monotonic
    int32_t min_hue = NA;
    int32_t max_hue = NA;
    int32_t orange = get_hue_or_value(hues, pb_const_color_orange);
    assert_greater(orange, &min_hue, &max_hue, 359);
    int32_t yellow = get_hue_or_value(hues, pb_const_color_yellow);
    assert_greater(yellow, &min_hue, &max_hue, 359);
    int32_t green = get_hue_or_value(hues, pb_const_color_green);
    assert_greater(green, &min_hue, &max_hue, 359);
    int32_t cyan = get_hue_or_value(hues, pb_const_color_cyan);
    assert_greater(cyan, &min_hue, &max_hue, 359);
    int32_t blue = get_hue_or_value(hues, pb_const_color_blue);
    assert_greater(blue, &min_hue, &max_hue, 359);
    int32_t purple = get_hue_or_value(hues, pb_const_color_purple);
    assert_greater(purple, &min_hue, &max_hue, 359);
    int32_t magenta = get_hue_or_value(hues, pb_const_color_magenta);
    assert_greater(magenta, &min_hue, &max_hue, 359);

    // If given, red must be the lowest or the highest hue
    int32_t red = get_hue_or_value(hues, pb_const_color_red);
    if (red != NA && (red < 0 || red > 359 || (red >= min_hue && red <= max_hue))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get user specified saturation threshold
    int32_t saturation_threshold = pb_obj_get_int(saturation);

    // Similarly to hues, now get user specified values and ensure their values are monotonic
    int32_t min_value = NA;
    int32_t max_value = NA;
    int32_t none = get_hue_or_value(values, mp_const_none);
    assert_greater(none, &min_value, &max_value, 100);
    int32_t black = get_hue_or_value(values, pb_const_color_black);
    assert_greater(black, &min_value, &max_value, 100);
    int32_t gray = get_hue_or_value(values, pb_const_color_gray);
    assert_greater(gray, &min_value, &max_value, 100);
    int32_t white = get_hue_or_value(values, pb_const_color_white);
    assert_greater(white, &min_value, &max_value, 100);

    // If all checks have passed, save the results.
    map->hue_red = red;
    map->hue_orange = orange;
    map->hue_yellow = yellow;
    map->hue_green = green;
    map->hue_cyan = cyan;
    map->hue_blue = blue;
    map->hue_purple = purple;
    map->hue_magenta = magenta;
    map->saturation_threshold = saturation_threshold;
    map->value_none = none;
    map->value_black = black;
    map->value_gray = gray;
    map->value_white = white;
}

// Generic class structure for ColorDistanceSensor
// Any color sensor structure with a color_map
// must have base and color_map as the first two members.
typedef struct _pb_ColorSensor_obj_t {
    mp_obj_base_t base;
    pb_hsv_map_t color_map;
} pb_ColorSensor_obj_t;

// pybricks._common.ColorDistanceSensor.color_map
STATIC mp_obj_t pupdevices_ColorDistanceSensor_color_map(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_NONE(hues),
        PB_ARG_DEFAULT_NONE(saturation),
        PB_ARG_DEFAULT_NONE(values));

    // If no arguments are given, return current map
    if (hues == mp_const_none && saturation == mp_const_none && values == mp_const_none) {
        return pack_color_map(&self->color_map);
    }

    // Otherwise, unpack given map
    unpack_color_map(&self->color_map, hues, saturation, values);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pb_ColorSensor_color_map_obj, 1, pupdevices_ColorDistanceSensor_color_map);
