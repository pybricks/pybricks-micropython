// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBHSV_H_
#define _PBHSV_H_

#define NA (-361)

typedef struct _pb_hsv_map_t {
    int32_t saturation_threshold;
    int32_t hue_red;
    int32_t hue_orange;
    int32_t hue_yellow;
    int32_t hue_green;
    int32_t hue_blue;
    int32_t hue_purple;
    int32_t value_none;
    int32_t value_black;
    int32_t value_white;
} pb_hsv_map_t;

int32_t bound_percentage(int32_t value);

void pb_hsv_map_save_default(pb_hsv_map_t *map);

mp_obj_t pack_color_map(pb_hsv_map_t *map);
void unpack_color_map(pb_hsv_map_t *map, mp_obj_t hues, mp_obj_t saturation, mp_obj_t values);

#endif // _PBHSV_H_
