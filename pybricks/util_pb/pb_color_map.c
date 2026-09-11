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

// This expands pbio_color_from_rgb with additional calibration steps that
// ultimately must be properly done in pbio_color_from_rgb, just like
// pbio_color_to_rgb, by adjusting RGB instead of hacking at the HSV value.
pbio_color_t pb_color_map_rgb_to_hsv(const pbio_color_rgb_t *rgb) {

    // Standard conversion
    pbio_color_t color = pbio_color_from_rgb(rgb);

    // Slight shift for lower hues to make yellow somewhat more accurate
    uint16_t h = pbio_color_get_h(color);
    if (h >= 350) {
        h = (350 + 2 * (h - 350)) % 360;
    } else if (h < 40) {
        h += 10;
    } else if (h < 60) {
        h = 50 + (h - 40) / 2;
    }

    return PBIO_COLOR_ENCODE(h, pbio_color_get_s(color), pbio_color_get_v(color));
}

// Gets the color map of the given port, with the default colors selected.
pbio_color_map_t *pb_color_map_init(pbio_port_t *port) {
    pbio_color_map_t *color_map;
    pb_assert(pbio_port_get_color_map(port, &color_map));
    pbio_color_map_set_default(color_map);
    return color_map;
}

// Get a discrete color that matches the given hsv values most closely
mp_obj_t pb_color_map_get_color(const pbio_color_map_t *color_map, pbio_color_t hsv) {
    if (color_map->num_colors == 0) {
        return mp_const_none;
    }
    return pb_type_Color_from_hsv(pbio_color_map_find(color_map, hsv));
}

mp_obj_t pb_color_map_detectable_colors_method(pbio_color_map_t *color_map, mp_obj_t colors_in) {

    // If no arguments are given, return current map
    if (colors_in == mp_const_none) {
        mp_obj_t colors[PBIO_COLOR_MAP_NUM_COLORS];
        for (uint8_t i = 0; i < color_map->num_colors; i++) {
            colors[i] = pb_type_Color_from_hsv(color_map->colors[i]);
        }
        return mp_obj_new_tuple(color_map->num_colors, colors);
    }

    // If arguments given, ensure all elements have the right type
    mp_obj_t *color_objs;
    size_t n;
    mp_obj_get_array(colors_in, &n, &color_objs);
    if (n > PBIO_COLOR_MAP_NUM_COLORS) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    pbio_color_t colors[PBIO_COLOR_MAP_NUM_COLORS];
    for (size_t i = 0; i < n; i++) {
        colors[i] = pb_type_Color_get_hsv(color_objs[i]);
    }

    // Save the given map
    pb_assert(pbio_color_map_set(color_map, colors, n));
    return mp_const_none;
}

#endif // PYBRICKS_PY_NXTDEVICES || PYBRICKS_PY_PUPDEVICES
