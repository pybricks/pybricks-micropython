// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_GEOMETRY_H
#define PYBRICKS_INCLUDED_PYBRICKS_GEOMETRY_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_GEOMETRY

#if MICROPY_PY_BUILTINS_FLOAT

#include "py/obj.h"

const mp_obj_module_t pb_module_geometry;

const mp_obj_type_t pb_type_Matrix;

mp_obj_t pb_type_Matrix_make_vector(size_t m, float *data, bool normalize);

mp_obj_t pb_type_Matrix_make_bitmap(size_t m, size_t n, float scale, uint32_t src);

float pb_type_Matrix_get_scalar(mp_obj_t self_in, size_t r, size_t c);

#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // PYBRICKS_PY_GEOMETRY

#endif // PYBRICKS_INCLUDED_PYBRICKS_GEOMETRY_H
