// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBHSV_H_
#define _PBHSV_H_

typedef struct _pb_hsv_map_t {
    int32_t saturation_threshold;
    int32_t hue_red;
    int32_t hue_orange;
    int32_t hue_yellow;
    int32_t hue_green;
    int32_t hue_cyan;
    int32_t hue_blue;
    int32_t hue_purple;
    int32_t hue_magenta;
    int32_t value_none;
    int32_t value_black;
    int32_t value_gray;
    int32_t value_white;
} pb_hsv_map_t;

mp_obj_t pb_hsv_get_color(pb_hsv_map_t *map, int32_t hue, int32_t saturation, int32_t value);

void pb_hsv_map_save_default(pb_hsv_map_t *map);

const mp_obj_fun_builtin_var_t pb_ColorSensor_color_map_obj;

#endif // _PBHSV_H_
