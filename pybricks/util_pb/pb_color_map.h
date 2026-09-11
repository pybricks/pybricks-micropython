// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PBHSV_H_
#define _PBHSV_H_

#include <pbio/color.h>
#include <pbio/color_map.h>
#include <pbio/port.h>

#include "py/obj.h"

pbio_color_t pb_color_map_rgb_to_hsv(const pbio_color_rgb_t *rgb);

pbio_color_map_t *pb_color_map_init(pbio_port_t *port);

mp_obj_t pb_color_map_get_color(const pbio_color_map_t *color_map, pbio_color_t hsv);

mp_obj_t pb_color_map_detectable_colors_method(pbio_color_map_t *color_map, mp_obj_t colors_in);

#endif // _PBHSV_H_
