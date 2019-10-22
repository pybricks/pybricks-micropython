// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef PYBRICKS_INCLUDED_PBOBJ_H
#define PYBRICKS_INCLUDED_PBOBJ_H

#include "py/obj.h"
#include <fixmath.h>

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

fix16_t pb_obj_get_fix16(mp_obj_t arg);

// Get value if object is not none, else return default
mp_int_t pb_obj_get_default_int(mp_obj_t obj, mp_int_t default_val);

// An enum element consists of a name and a value
typedef struct _pb_obj_enum_elem_t {
    mp_obj_base_t base;
    uint16_t name;
    int32_t value;
} pb_obj_enum_elem_t;

// Macro to produce element of an enum table
#define MP_ROM_ENUM_ELEM(enum_type, enum_elem_name, enum_elem_value) \
    { \
        MP_ROM_QSTR(enum_elem_name), \
        MP_ROM_PTR(( \
            &(pb_obj_enum_elem_t) { \
                {&enum_type}, \
                .name = enum_elem_name, \
                .value = enum_elem_value \
            }\
        )) \
    }

#endif // PYBRICKS_INCLUDED_PBOBJ_H
