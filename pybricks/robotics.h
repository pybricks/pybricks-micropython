// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
#define PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS

#include "py/obj.h"

#if MICROPY_PY_BUILTINS_FLOAT

const mp_obj_type_t pb_type_Matrix_type;

const mp_obj_fun_builtin_var_t pb_func_Vector;
const mp_obj_fun_builtin_var_t pb_func_UnitVector;

#endif // MICROPY_PY_BUILTINS_FLOAT

const mp_obj_type_t pb_type_drivebase;

const mp_obj_module_t pb_module_robotics;

#endif // PYBRICKS_PY_ROBOTICS

#endif // PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
