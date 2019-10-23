// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef PYBRICKS_INCLUDED_MODENUM_H
#define PYBRICKS_INCLUDED_MODENUM_H

#include "py/obj.h"

void enum_class_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind);

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

#endif // PYBRICKS_INCLUDED_MODENUM_H
