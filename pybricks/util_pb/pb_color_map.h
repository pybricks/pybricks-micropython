// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBHSV_H_
#define _PBHSV_H_

#include <pbio/color.h>

#include "py/obj.h"

void color_map_rgb_to_hsv(const pbio_color_rgb_t *rgb, pbio_color_hsv_t *hsv);

void pb_color_map_save_default(mp_obj_t *color_map);

mp_obj_t pb_color_map_get_color(mp_obj_t *color_map, pbio_color_hsv_t *hsv);

MP_DECLARE_CONST_FUN_OBJ_KW(pb_ColorSensor_color_map_obj);

#endif // _PBHSV_H_
