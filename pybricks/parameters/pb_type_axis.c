// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/control.h>

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_type_enum.h>
#include <pybricks/tools/pb_type_matrix.h>

#if MICROPY_PY_BUILTINS_FLOAT

static const float pb_type_Axis_X_data[] = {1.0f, 0.0f, 0.0f};

const pb_type_Matrix_obj_t pb_type_Axis_X_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_type_Axis_X_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

static const float pb_type_Axis_Y_data[] = {0.0f, 1.0f, 0.0f};

const pb_type_Matrix_obj_t pb_type_Axis_Y_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_type_Axis_Y_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

static const float pb_type_Axis_Z_data[] = {0.0f, 0.0f, 1.0f};

const pb_type_Matrix_obj_t pb_type_Axis_Z_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_type_Axis_Z_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};
#endif // MICROPY_PY_BUILTINS_FLOAT

static const mp_rom_map_elem_t pb_type_Axis_table[] = {
    #if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_X),     MP_ROM_PTR(&pb_type_Axis_X_obj)},
    { MP_ROM_QSTR(MP_QSTR_Y),     MP_ROM_PTR(&pb_type_Axis_Y_obj)},
    { MP_ROM_QSTR(MP_QSTR_Z),     MP_ROM_PTR(&pb_type_Axis_Z_obj)},
    #else
    { MP_ROM_QSTR(MP_QSTR_X),     MP_ROM_INT(pb_type_Axis_X_int_enum)},
    { MP_ROM_QSTR(MP_QSTR_Y),     MP_ROM_INT(pb_type_Axis_Y_int_enum)},
    { MP_ROM_QSTR(MP_QSTR_Z),     MP_ROM_INT(pb_type_Axis_Z_int_enum)},
    #endif // MICROPY_PY_BUILTINS_FLOAT
};
static MP_DEFINE_CONST_DICT(pb_type_Axis_locals_dict, pb_type_Axis_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_enum_type_Axis,
    MP_QSTR_Axis,
    MP_TYPE_FLAG_NONE,
    locals_dict, &(pb_type_Axis_locals_dict));

#endif // PYBRICKS_PY_PARAMETERS
