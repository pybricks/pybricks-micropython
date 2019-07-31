// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef PYBRICKS_INCLUDED_PBOBJ_H
#define PYBRICKS_INCLUDED_PBOBJ_H

#include "py/obj.h"

// Shortcut for the equivalent of: import name
#define PB_IMPORT_MODULE(name) \
mp_store_global(name, mp_import_name(name, mp_const_none, MP_OBJ_NEW_SMALL_INT(0)))

// Shortcut for the equivalent of: from name import *
#define PB_FROM_MODULE_IMPORT_ALL(name) \
mp_import_all(mp_import_name(name, mp_const_none, MP_OBJ_NEW_SMALL_INT(0)))

// Shortcut for converting table to Enum-like class object
#define PB_DEFINE_CONST_ENUM(enum_name, table_name) \
MP_DEFINE_CONST_DICT(enum_name ## _locals_dict, table_name); \
const mp_obj_type_t enum_name = { \
    { &mp_type_type }, \
    .locals_dict = (mp_obj_dict_t*)&(enum_name ## _locals_dict),\
}

// Shortcut for defining attribute as address offset from base
#define PB_ATTR(type, elem) &(mp_int_t){offsetof(type, elem)}

// like mp_obj_get_int() but also allows float
#if MICROPY_PY_BUILTINS_FLOAT
mp_int_t pb_obj_get_int(mp_obj_t arg);
#else
#define pb_obj_get_int mp_obj_get_int
#endif

#endif // PYBRICKS_INCLUDED_PBOBJ_H
