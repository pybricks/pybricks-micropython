// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_PARAMETERS_H
#define PYBRICKS_INCLUDED_PYBRICKS_PARAMETERS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/button.h>
#include <pbio/color.h>

#include "py/obj.h"

#include <pybricks/util_mp/pb_type_enum.h>
#include <pybricks/tools/pb_type_matrix.h>

extern const mp_obj_type_t pb_enum_type_Axis;
#if MICROPY_PY_BUILTINS_FLOAT
extern const pb_type_Matrix_obj_t pb_type_Axis_X_obj;
extern const pb_type_Matrix_obj_t pb_type_Axis_Y_obj;
extern const pb_type_Matrix_obj_t pb_type_Axis_Z_obj;
#else
// Systems without floating point don't support vector math but the
// axes are still used as setup parameters. We use nonzero int indexes
// so that they may still be negated to get the opposite direction.
#define pb_type_Axis_X_int_enum (1)
#define pb_type_Axis_Y_int_enum (2)
#define pb_type_Axis_Z_int_enum (3)
#endif // MICROPY_PY_BUILTINS_FLOAT

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

#endif // PYBRICKS_PY_PARAMETERS

#endif // PYBRICKS_INCLUDED_PYBRICKS_PARAMETERS_H
