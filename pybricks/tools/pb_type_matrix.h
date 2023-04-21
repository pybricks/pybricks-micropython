// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_TOOLS_MATRIX_H
#define PYBRICKS_INCLUDED_PYBRICKS_TOOLS_MATRIX_H

#include "py/mpconfig.h"

#if MICROPY_PY_BUILTINS_FLOAT

#include "py/obj.h"

extern const mp_obj_type_t pb_type_Matrix;

typedef struct _pb_type_Matrix_obj_t {
    mp_obj_base_t base;
    float *data;
    float scale;
    size_t m;
    size_t n;
    bool transposed;
} pb_type_Matrix_obj_t;

mp_obj_t pb_type_Matrix_make_vector(size_t m, float *data, bool normalize);

mp_obj_t pb_type_Matrix_make_bitmap(size_t m, size_t n, float scale, uint32_t src);

float pb_type_Matrix_get_scalar(mp_obj_t self_in, size_t r, size_t c);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(pb_geometry_vector_obj);

MP_DECLARE_CONST_FUN_OBJ_2(pb_type_matrix_cross_obj);

#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // PYBRICKS_INCLUDED_PYBRICKS_TOOLS_MATRIX_H
