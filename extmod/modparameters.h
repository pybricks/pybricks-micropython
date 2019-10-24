// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef PYBRICKS_INCLUDED_MODPARAMETERS_H
#define PYBRICKS_INCLUDED_MODPARAMETERS_H

#include "modenum.h"

const mp_obj_type_t pb_enum_type_Port;

const mp_obj_type_t pb_enum_type_Button;

// Stop Enum
const mp_obj_type_t pb_enum_type_Stop;
const pb_obj_enum_elem_t pb_const_coast;
const pb_obj_enum_elem_t pb_const_brake;
const pb_obj_enum_elem_t pb_const_hold;

// Direction Enum
const mp_obj_type_t pb_enum_type_Direction;
const pb_obj_enum_elem_t pb_const_clockwise;
const pb_obj_enum_elem_t pb_const_counterclockwise;

// Color enum
const mp_obj_type_t pb_enum_type_Color;
const pb_obj_enum_elem_t pb_const_black;
const pb_obj_enum_elem_t pb_const_purple;
const pb_obj_enum_elem_t pb_const_blue;
const pb_obj_enum_elem_t pb_const_green;
const pb_obj_enum_elem_t pb_const_yellow;
const pb_obj_enum_elem_t pb_const_orange;
const pb_obj_enum_elem_t pb_const_red;
const pb_obj_enum_elem_t pb_const_white;
const pb_obj_enum_elem_t pb_const_brown;

#endif // PYBRICKS_INCLUDED_MODPARAMETERS_H
