// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBHSV_H_
#define _PBHSV_H_

#include <pbio/color.h>

#include "py/obj.h"

void pb_color_map_rgb_to_hsv(const pbio_color_rgb_t *rgb, pbio_color_hsv_t *hsv);

void pb_color_map_save_default(mp_obj_t *color_map);

mp_obj_t pb_color_map_get_color(mp_obj_t *color_map, pbio_color_hsv_t *hsv);

mp_obj_t pb_color_map_detectable_colors_method(mp_obj_t *self_color_map, mp_obj_t colors_in);

#endif // _PBHSV_H_
