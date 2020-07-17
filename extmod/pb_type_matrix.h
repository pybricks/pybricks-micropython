#ifndef PYBRICKS_INCLUDED_PY_PB_TYPE_MATRIX_H
#define PYBRICKS_INCLUDED_PY_PB_TYPE_MATRIX_H

#include "py/obj.h"

#if MICROPY_PY_BUILTINS_FLOAT

const mp_obj_type_t pb_type_Matrix_type;

const mp_obj_fun_builtin_var_t robotics_Vector_obj;
const mp_obj_fun_builtin_var_t robotics_UnitVector_obj;

#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // PYBRICKS_INCLUDED_PY_PB_TYPE_MATRIX_H
