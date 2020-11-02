// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_GEOMETRY

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/geometry.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.geometry.Vector
STATIC mp_obj_t robotics_Vector(size_t n_args, const mp_obj_t *args) {
    return pb_type_Matrix_make_vector(n_args, args, false);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_func_Vector_obj, 2, 4, robotics_Vector);

// pybricks.geometry.UnitVector
STATIC mp_obj_t robotics_UnitVector(size_t n_args, const mp_obj_t *args) {
    return pb_type_Matrix_make_vector(n_args, args, true);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_func_UnitVector_obj, 2, 4, robotics_UnitVector);

STATIC const mp_rom_map_elem_t geometry_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_Matrix),      MP_ROM_PTR(&pb_type_Matrix)        },
    { MP_ROM_QSTR(MP_QSTR_Vector),      MP_ROM_PTR(&pb_func_Vector_obj)    },
    { MP_ROM_QSTR(MP_QSTR_UnitVector),  MP_ROM_PTR(&pb_func_UnitVector_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_module_geometry_globals, geometry_globals_table);

const mp_obj_module_t pb_module_geometry = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_geometry_globals,
};

#endif // PYBRICKS_PY_GEOMETRY
