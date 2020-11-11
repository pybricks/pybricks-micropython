// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_PARAMETERS_H
#define PYBRICKS_INCLUDED_PYBRICKS_PARAMETERS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/color.h>

#include "py/obj.h"

#include <pybricks/util_mp/pb_type_enum.h>

#if PYBRICKS_PY_PARAMETERS_BUTTON

const mp_obj_type_t pb_enum_type_Button;

const pb_obj_enum_member_t pb_Button_UP_obj;
const pb_obj_enum_member_t pb_Button_DOWN_obj;
const pb_obj_enum_member_t pb_Button_LEFT_obj;
const pb_obj_enum_member_t pb_Button_RIGHT_obj;
const pb_obj_enum_member_t pb_Button_CENTER_obj;
const pb_obj_enum_member_t pb_Button_LEFT_UP_obj;
const pb_obj_enum_member_t pb_Button_LEFT_DOWN_obj;
const pb_obj_enum_member_t pb_Button_RIGHT_UP_obj;
const pb_obj_enum_member_t pb_Button_RIGHT_DOWN_obj;
#if PYBRICKS_HUB_EV3BRICK
const pb_obj_enum_member_t pb_Button_BEACON_obj;
#endif
#if PYBRICKS_HUB_PRIMEHUB
const pb_obj_enum_member_t pb_Button_BT_obj;
#endif

#endif // PYBRICKS_PY_PARAMETERS_BUTTON

const mp_obj_type_t pb_type_Color;

typedef struct _pb_type_Color_obj_t {
    mp_obj_base_t base;
    mp_obj_t name;
    pbio_color_hsv_t hsv;
} pb_type_Color_obj_t;

pb_type_Color_obj_t *pb_type_Color_new_empty();
const pbio_color_hsv_t *pb_type_Color_get_hsv(mp_obj_t obj);

const pb_type_Color_obj_t pb_Color_RED_obj;
const pb_type_Color_obj_t pb_Color_BROWN_obj;
const pb_type_Color_obj_t pb_Color_ORANGE_obj;
const pb_type_Color_obj_t pb_Color_YELLOW_obj;
const pb_type_Color_obj_t pb_Color_GREEN_obj;
const pb_type_Color_obj_t pb_Color_CYAN_obj;
const pb_type_Color_obj_t pb_Color_BLUE_obj;
const pb_type_Color_obj_t pb_Color_VIOLET_obj;
const pb_type_Color_obj_t pb_Color_MAGENTA_obj;
const pb_type_Color_obj_t pb_Color_NONE_obj;
const pb_type_Color_obj_t pb_Color_BLACK_obj;
const pb_type_Color_obj_t pb_Color_GRAY_obj;
const pb_type_Color_obj_t pb_Color_WHITE_obj;

const mp_obj_type_t pb_enum_type_Direction;

const pb_obj_enum_member_t pb_Direction_CLOCKWISE_obj;
const pb_obj_enum_member_t pb_Direction_COUNTERCLOCKWISE_obj;

const mp_obj_base_t pb_Icon_obj;

const mp_obj_type_t pb_enum_type_Port;

const mp_obj_type_t pb_enum_type_Stop;

const pb_obj_enum_member_t pb_Stop_COAST_obj;
const pb_obj_enum_member_t pb_Stop_BRAKE_obj;
const pb_obj_enum_member_t pb_Stop_HOLD_obj;

const mp_obj_type_t pb_enum_type_Side;

const pb_obj_enum_member_t pb_Side_BACK_obj;
const pb_obj_enum_member_t pb_Side_BOTTOM_obj;
const pb_obj_enum_member_t pb_Side_FRONT_obj;
const pb_obj_enum_member_t pb_Side_LEFT_obj;
const pb_obj_enum_member_t pb_Side_RIGHT_obj;
const pb_obj_enum_member_t pb_Side_TOP_obj;

const mp_obj_module_t pb_module_parameters;

#endif // PYBRICKS_PY_PARAMETERS

#endif // PYBRICKS_INCLUDED_PYBRICKS_PARAMETERS_H
