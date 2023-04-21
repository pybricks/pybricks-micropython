// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS && MICROPY_PY_BUILTINS_FLOAT

#include <pbio/control.h>

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_type_enum.h>
#include <pybricks/tools/pb_type_matrix.h>

STATIC const float pb_type_Axis_X_data[] = {1.0f, 0.0f, 0.0f};

const pb_type_Matrix_obj_t pb_type_Axis_X_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_type_Axis_X_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

STATIC const float pb_type_Axis_Y_data[] = {0.0f, 1.0f, 0.0f};

const pb_type_Matrix_obj_t pb_type_Axis_Y_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_type_Axis_Y_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

STATIC const float pb_type_Axis_Z_data[] = {0.0f, 0.0f, 1.0f};

const pb_type_Matrix_obj_t pb_type_Axis_Z_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_type_Axis_Z_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

STATIC const mp_rom_map_elem_t pb_type_Axis_table[] = {
    { MP_ROM_QSTR(MP_QSTR_X),     MP_ROM_PTR(&pb_type_Axis_X_obj)},
    { MP_ROM_QSTR(MP_QSTR_Y),     MP_ROM_PTR(&pb_type_Axis_Y_obj)},
    { MP_ROM_QSTR(MP_QSTR_Z),     MP_ROM_PTR(&pb_type_Axis_Z_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_type_Axis_locals_dict, pb_type_Axis_table);

const mp_obj_type_t pb_enum_type_Axis = {
    { &mp_type_type },
    .name = MP_QSTR_Axis,
    .locals_dict = (mp_obj_dict_t *)&(pb_type_Axis_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS && MICROPY_PY_BUILTINS_FLOAT
