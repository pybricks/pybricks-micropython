// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

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


#endif // PYBRICKS_INCLUDED_PY_PB_TYPE_ENUM_H
