// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef PYBRICKS_INCLUDED_PBOBJ_H
#define PYBRICKS_INCLUDED_PBOBJ_H

#include "py/obj.h"

// Shortcut for converting table to Enum-like class object
#define PB_DEFINE_CONST_ENUM(enum_name, table_name) \
MP_DEFINE_CONST_DICT(enum_name ## _locals_dict, table_name); \
const mp_obj_type_t enum_name = { \
    { &mp_type_type }, \
    .locals_dict = (mp_obj_dict_t*)&(enum_name ## _locals_dict),\
}

// Get a real-valued number as float object if supported and as integer object otherwise
#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_NONE
    #define mp_obj_get_num mp_obj_get_int
#else
    #define mp_obj_get_num mp_obj_get_float
#endif //MICROPY_FLOAT_IMPL

#endif // PYBRICKS_INCLUDED_PBOBJ_H
