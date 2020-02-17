// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef PYBRICKS_INCLUDED_MODPARAMETERS_H
#define PYBRICKS_INCLUDED_MODPARAMETERS_H

#include "pb_type_enum.h"

#if PYBRICKS_PY_PARAMETERS

const mp_obj_type_t pb_enum_type_Port;

// Button Enum
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
const pb_obj_enum_member_t pb_Button_BEACON_obj;
#define pb_const_button_up          MP_OBJ_FROM_PTR(&pb_Button_UP_obj)
#define pb_const_button_down        MP_OBJ_FROM_PTR(&pb_Button_DOWN_obj)
#define pb_const_button_left        MP_OBJ_FROM_PTR(&pb_Button_LEFT_obj)
#define pb_const_button_right       MP_OBJ_FROM_PTR(&pb_Button_RIGHT_obj)
#define pb_const_button_center      MP_OBJ_FROM_PTR(&pb_Button_CENTER_obj)
#define pb_const_button_left_up     MP_OBJ_FROM_PTR(&pb_Button_LEFT_UP_obj)
#define pb_const_button_left_down   MP_OBJ_FROM_PTR(&pb_Button_LEFT_DOWN_obj)
#define pb_const_button_right_up    MP_OBJ_FROM_PTR(&pb_Button_RIGHT_UP_obj)
#define pb_const_button_right_down  MP_OBJ_FROM_PTR(&pb_Button_RIGHT_DOWN_obj)
#define pb_const_button_beacon      MP_OBJ_FROM_PTR(&pb_Button_BEACON_obj)


// Stop Enum
const mp_obj_type_t pb_enum_type_Stop;
const pb_obj_enum_member_t pb_Stop_COAST_obj;
const pb_obj_enum_member_t pb_Stop_BRAKE_obj;
const pb_obj_enum_member_t pb_Stop_HOLD_obj;
#define pb_const_stop_coast MP_OBJ_FROM_PTR(&pb_Stop_COAST_obj)
#define pb_const_stop_brake MP_OBJ_FROM_PTR(&pb_Stop_BRAKE_obj)
#define pb_const_stop_hold  MP_OBJ_FROM_PTR(&pb_Stop_HOLD_obj)

// Direction Enum
const mp_obj_type_t pb_enum_type_Direction;
const pb_obj_enum_member_t pb_Direction_CLOCKWISE_obj;
const pb_obj_enum_member_t pb_Direction_COUNTERCLOCKWISE_obj;
#define pb_const_direction_clockwise        MP_OBJ_FROM_PTR(&pb_Direction_CLOCKWISE_obj)
#define pb_const_direction_counterclockwise MP_OBJ_FROM_PTR(&pb_Direction_COUNTERCLOCKWISE_obj)

// Color enum
const mp_obj_type_t pb_enum_type_Color;
const pb_obj_enum_member_t pb_Color_BLACK_obj;
const pb_obj_enum_member_t pb_Color_PURPLE_obj;
const pb_obj_enum_member_t pb_Color_BLUE_obj;
const pb_obj_enum_member_t pb_Color_GREEN_obj;
const pb_obj_enum_member_t pb_Color_YELLOW_obj;
const pb_obj_enum_member_t pb_Color_ORANGE_obj;
const pb_obj_enum_member_t pb_Color_RED_obj;
const pb_obj_enum_member_t pb_Color_WHITE_obj;
const pb_obj_enum_member_t pb_Color_BROWN_obj;
#define pb_const_color_black    MP_OBJ_FROM_PTR(&pb_Color_BLACK_obj)
#define pb_const_color_purple   MP_OBJ_FROM_PTR(&pb_Color_PURPLE_obj)
#define pb_const_color_blue     MP_OBJ_FROM_PTR(&pb_Color_BLUE_obj)
#define pb_const_color_green    MP_OBJ_FROM_PTR(&pb_Color_GREEN_obj)
#define pb_const_color_yellow   MP_OBJ_FROM_PTR(&pb_Color_YELLOW_obj)
#define pb_const_color_orange   MP_OBJ_FROM_PTR(&pb_Color_ORANGE_obj)
#define pb_const_color_red      MP_OBJ_FROM_PTR(&pb_Color_RED_obj)
#define pb_const_color_white    MP_OBJ_FROM_PTR(&pb_Color_WHITE_obj)
#define pb_const_color_brown    MP_OBJ_FROM_PTR(&pb_Color_BROWN_obj)

#endif // PYBRICKS_PY_PARAMETERS

#endif // PYBRICKS_INCLUDED_MODPARAMETERS_H
