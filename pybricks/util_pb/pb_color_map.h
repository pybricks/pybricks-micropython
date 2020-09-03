// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBHSV_H_
#define _PBHSV_H_

void pb_color_map_save_default(mp_obj_t *color_map);

mp_obj_t pb_color_map_get_color(mp_obj_t *color_map, pbio_color_hsv_t *hsv);

const mp_obj_fun_builtin_var_t pb_ColorSensor_color_map_obj;

#endif // _PBHSV_H_
