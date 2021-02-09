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

extern const mp_obj_type_t pb_enum_type_Button;

extern const pb_obj_enum_member_t pb_Button_UP_obj;
extern const pb_obj_enum_member_t pb_Button_DOWN_obj;
extern const pb_obj_enum_member_t pb_Button_LEFT_obj;
extern const pb_obj_enum_member_t pb_Button_RIGHT_obj;
extern const pb_obj_enum_member_t pb_Button_RIGHT_PLUS_obj;
extern const pb_obj_enum_member_t pb_Button_RIGHT_MINUS_obj;
extern const pb_obj_enum_member_t pb_Button_CENTER_obj;
extern const pb_obj_enum_member_t pb_Button_LEFT_UP_obj;
extern const pb_obj_enum_member_t pb_Button_LEFT_DOWN_obj;
extern const pb_obj_enum_member_t pb_Button_LEFT_PLUS_obj;
extern const pb_obj_enum_member_t pb_Button_LEFT_MINUS_obj;
extern const pb_obj_enum_member_t pb_Button_RIGHT_UP_obj;
extern const pb_obj_enum_member_t pb_Button_RIGHT_DOWN_obj;
extern const pb_obj_enum_member_t pb_Button_BEACON_obj;
extern const pb_obj_enum_member_t pb_Button_BLUETOOTH_obj;

#endif // PYBRICKS_PY_PARAMETERS_BUTTON

extern const mp_obj_type_t pb_type_Color;
extern const mp_obj_base_t pb_type_Color_obj;

typedef struct _pb_type_Color_obj_t {
    mp_obj_base_t base;
    pbio_color_hsv_t hsv;
} pb_type_Color_obj_t;

pb_type_Color_obj_t *pb_type_Color_new_empty(void);
const pbio_color_hsv_t *pb_type_Color_get_hsv(mp_obj_t obj);
void pb_type_Color_reset(void);

extern const pb_type_Color_obj_t pb_Color_RED_obj;
extern const pb_type_Color_obj_t pb_Color_BROWN_obj;
extern const pb_type_Color_obj_t pb_Color_ORANGE_obj;
extern const pb_type_Color_obj_t pb_Color_YELLOW_obj;
extern const pb_type_Color_obj_t pb_Color_GREEN_obj;
extern const pb_type_Color_obj_t pb_Color_CYAN_obj;
extern const pb_type_Color_obj_t pb_Color_BLUE_obj;
extern const pb_type_Color_obj_t pb_Color_VIOLET_obj;
extern const pb_type_Color_obj_t pb_Color_MAGENTA_obj;
extern const pb_type_Color_obj_t pb_Color_NONE_obj;
extern const pb_type_Color_obj_t pb_Color_BLACK_obj;
extern const pb_type_Color_obj_t pb_Color_GRAY_obj;
extern const pb_type_Color_obj_t pb_Color_WHITE_obj;

extern const mp_obj_type_t pb_enum_type_Direction;

extern const pb_obj_enum_member_t pb_Direction_CLOCKWISE_obj;
extern const pb_obj_enum_member_t pb_Direction_COUNTERCLOCKWISE_obj;

#if PYBRICKS_PY_PARAMETERS_ICON
extern const mp_obj_base_t pb_Icon_obj;
#endif

extern const mp_obj_type_t pb_enum_type_Port;

extern const mp_obj_type_t pb_enum_type_Stop;

extern const pb_obj_enum_member_t pb_Stop_COAST_obj;
extern const pb_obj_enum_member_t pb_Stop_BRAKE_obj;
extern const pb_obj_enum_member_t pb_Stop_HOLD_obj;

extern const mp_obj_type_t pb_enum_type_Side;

extern const pb_obj_enum_member_t pb_Side_BACK_obj;
extern const pb_obj_enum_member_t pb_Side_BOTTOM_obj;
extern const pb_obj_enum_member_t pb_Side_FRONT_obj;
extern const pb_obj_enum_member_t pb_Side_LEFT_obj;
extern const pb_obj_enum_member_t pb_Side_RIGHT_obj;
extern const pb_obj_enum_member_t pb_Side_TOP_obj;

extern const mp_obj_module_t pb_module_parameters;

#endif // PYBRICKS_PY_PARAMETERS

#endif // PYBRICKS_INCLUDED_PYBRICKS_PARAMETERS_H
