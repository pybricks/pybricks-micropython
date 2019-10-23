// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef PYBRICKS_INCLUDED_MODENUM_H
#define PYBRICKS_INCLUDED_MODENUM_H

#include "py/obj.h"

void enum_class_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind);

// returns enum->value if enum is of expected type, throws TypeError otherwise
mp_int_t enum_get_value_maybe(mp_obj_t enum_elem, const mp_obj_type_t *valid_type);

// An enum element consists of a name and a value
typedef struct _pb_obj_enum_elem_t {
    mp_obj_base_t base;
    uint16_t name;
    mp_int_t value;
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

// Shortcut for converting table to Enum-like class object
#define PB_DEFINE_ENUM(enum_type, enum_name, table_name) \
STATIC MP_DEFINE_CONST_DICT(enum_type ## _locals_dict, table_name); \
const mp_obj_type_t enum_type = { \
    { &mp_type_type }, \
    .name = enum_name, \
    .print = enum_class_print, \
    .locals_dict = (mp_obj_dict_t*)&(enum_type ## _locals_dict),\
}

#endif // PYBRICKS_INCLUDED_MODENUM_H
