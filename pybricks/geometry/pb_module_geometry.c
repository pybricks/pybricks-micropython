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
#include <pybricks/util_mp/pb_type_enum.h>

STATIC const float pb_Axis_x_data[] = {1.0f, 0.0f, 0.0f};

const pb_type_Matrix_obj_t pb_Axis_X_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_Axis_x_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

STATIC const float pb_Axis_y_data[] = {0.0f, 1.0f, 0.0f};

const pb_type_Matrix_obj_t pb_Axis_Y_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_Axis_y_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

STATIC const float pb_Axis_z_data[] = {0.0f, 0.0f, 1.0f};

const pb_type_Matrix_obj_t pb_Axis_Z_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_Axis_z_data,
    .scale = 1.0f,
    .m = 3,
    .n = 1,
};

STATIC const float pb_Matrix_identity3_data[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
};

const pb_type_Matrix_obj_t pb_Matrix_identity3_obj = {
    {&pb_type_Matrix},
    .data = (float *)pb_Matrix_identity3_data,
    .scale = 1.0f,
    .m = 3,
    .n = 3,
};

STATIC const mp_rom_map_elem_t pb_Axis_table[] = {
    { MP_ROM_QSTR(MP_QSTR_X),     MP_ROM_PTR(&pb_Axis_X_obj)},
    { MP_ROM_QSTR(MP_QSTR_Y),     MP_ROM_PTR(&pb_Axis_Y_obj)},
    { MP_ROM_QSTR(MP_QSTR_Z),     MP_ROM_PTR(&pb_Axis_Z_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_type_Axis_locals_dict, pb_Axis_table);

const mp_obj_type_t pb_enum_type_Axis = {
    { &mp_type_type },
    .name = MP_QSTR_Axis,
    .locals_dict = (mp_obj_dict_t *)&(pb_type_Axis_locals_dict),
};

// pybricks.geometry.vector
STATIC mp_obj_t vector(size_t n_args, const mp_obj_t *args) {

    // Convert user object to floats
    float data[3];
    for (size_t i = 0; i < n_args; i++) {
        data[i] = mp_obj_get_float_to_f(args[i]);
    }

    // Create and return Matrix object with vector shape
    return pb_type_Matrix_make_vector(n_args, data, false);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_func_vector_obj, 2, 3, vector);

// pybricks.geometry.cross
STATIC mp_obj_t pb_func_cross(mp_obj_t a_in, mp_obj_t b_in) {

    // Get a and b vectors
    pb_type_Matrix_obj_t *a = MP_OBJ_TO_PTR(a_in);
    pb_type_Matrix_obj_t *b = MP_OBJ_TO_PTR(b_in);

    // Verify matching dimensions else raise error
    if (a->n * a->m != 3 || b->n * b->m != 3) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Create c vector.
    pb_type_Matrix_obj_t *c = mp_obj_malloc(pb_type_Matrix_obj_t, &pb_type_Matrix);
    c->m = 3;
    c->n = 1;
    c->data = m_new(float, 3);

    // Evaluate cross product
    c->data[0] = a->data[1] * b->data[2] - a->data[2] * b->data[1];
    c->data[1] = a->data[2] * b->data[0] - a->data[0] * b->data[2];
    c->data[2] = a->data[0] * b->data[1] - a->data[1] * b->data[0];
    c->scale = a->scale * b->scale;

    return MP_OBJ_FROM_PTR(c);
}
MP_DEFINE_CONST_FUN_OBJ_2(pb_func_cross_obj, pb_func_cross);

STATIC const mp_rom_map_elem_t geometry_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_Axis),        MP_ROM_PTR(&pb_enum_type_Axis)     },
    { MP_ROM_QSTR(MP_QSTR_Matrix),      MP_ROM_PTR(&pb_type_Matrix)        },
    { MP_ROM_QSTR(MP_QSTR_vector),      MP_ROM_PTR(&pb_func_vector_obj)    },
    { MP_ROM_QSTR(MP_QSTR_cross),       MP_ROM_PTR(&pb_func_cross_obj)     },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_geometry_globals, geometry_globals_table);

const mp_obj_module_t pb_module_geometry = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_geometry_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_geometry, pb_module_geometry);

#endif // PYBRICKS_PY_GEOMETRY
