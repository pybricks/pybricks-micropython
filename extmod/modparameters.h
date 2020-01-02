// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef PYBRICKS_INCLUDED_MODPARAMETERS_H
#define PYBRICKS_INCLUDED_MODPARAMETERS_H

#include "pb_type_enum.h"

#if PYBRICKS_PY_PARAMETERS

const mp_obj_type_t pb_enum_type_Port;

// Button Enum
const mp_obj_type_t pb_enum_type_Button;
const pb_obj_enum_member_t pb_const_btn_up;
const pb_obj_enum_member_t pb_const_btn_down;
const pb_obj_enum_member_t pb_const_btn_left;
const pb_obj_enum_member_t pb_const_btn_right;
const pb_obj_enum_member_t pb_const_btn_center;
const pb_obj_enum_member_t pb_const_btn_left_up;
const pb_obj_enum_member_t pb_const_btn_left_down;
const pb_obj_enum_member_t pb_const_btn_right_up;
const pb_obj_enum_member_t pb_const_btn_right_down;
const pb_obj_enum_member_t pb_const_btn_beacon;

// Stop Enum
const mp_obj_type_t pb_enum_type_Stop;
const pb_obj_enum_member_t pb_const_coast;
const pb_obj_enum_member_t pb_const_brake;
const pb_obj_enum_member_t pb_const_hold;

// Direction Enum
const mp_obj_type_t pb_enum_type_Direction;
const pb_obj_enum_member_t pb_const_clockwise;
const pb_obj_enum_member_t pb_const_counterclockwise;

// Color enum
const mp_obj_type_t pb_enum_type_Color;
const pb_obj_enum_member_t pb_const_black;
const pb_obj_enum_member_t pb_const_purple;
const pb_obj_enum_member_t pb_const_blue;
const pb_obj_enum_member_t pb_const_green;
const pb_obj_enum_member_t pb_const_yellow;
const pb_obj_enum_member_t pb_const_orange;
const pb_obj_enum_member_t pb_const_red;
const pb_obj_enum_member_t pb_const_white;
const pb_obj_enum_member_t pb_const_brown;

#endif // PYBRICKS_PY_PARAMETERS

#endif // PYBRICKS_INCLUDED_MODPARAMETERS_H
