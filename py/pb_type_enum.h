// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef PYBRICKS_INCLUDED_PY_PB_TYPE_ENUM_H
#define PYBRICKS_INCLUDED_PY_PB_TYPE_ENUM_H

#include "py/obj.h"

void pb_type_enum_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);

// returns enum->value if enum is of expected type, throws TypeError otherwise
mp_int_t pb_type_enum_get_value(mp_obj_t obj, const mp_obj_type_t *type);

// An enum member consists of a name and a value
typedef struct _pb_obj_enum_member_t {
    mp_obj_base_t base;
    uint16_t name;
    mp_int_t value;
} pb_obj_enum_member_t;

// Macro to produce member of an enum table
#define PB_ROM_ENUM_MEMBER(enum_type, member_name, member_value) \
    { \
        MP_ROM_QSTR(member_name), \
        MP_ROM_PTR(( \
            &(pb_obj_enum_member_t) { \
                {&enum_type}, \
                .name = member_name, \
                .value = member_value \
            }\
        )) \
    }

// Shortcut for converting table to Enum-like class object
#define PB_DEFINE_ENUM(enum_type, enum_name, table_name) \
STATIC MP_DEFINE_CONST_DICT(enum_type ## _locals_dict, table_name); \
const mp_obj_type_t enum_type = { \
    { &mp_type_type }, \
    .name = enum_name, \
    .print = pb_type_enum_print, \
    .unary_op = mp_generic_unary_op, \
    .locals_dict = (mp_obj_dict_t*)&(enum_type ## _locals_dict),\
}

#endif // PYBRICKS_INCLUDED_PY_PB_TYPE_ENUM_H
