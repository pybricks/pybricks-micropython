// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
#define PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS

#include <math.h>

#include "py/obj.h"

#if MICROPY_PY_BUILTINS_FLOAT

const mp_obj_type_t pb_type_Matrix_type;
float pb_type_Matrix__get_scalar(mp_obj_t self_in, size_t r, size_t c);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(pb_func_Vector_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(pb_func_UnitVector_obj);

#endif // MICROPY_PY_BUILTINS_FLOAT

const mp_obj_type_t pb_type_drivebase;

const mp_obj_module_t pb_module_robotics;

#endif // PYBRICKS_PY_ROBOTICS

#endif // PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
