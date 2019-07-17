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

// Parse given positional and keyword arguments against a list of allowed arguments
// First n_ignore arguments are required arguments for which no keyword can be given.
#define PB_PARSE_ARGS(parsed_args, n_args, pos_args, kw_args, allowed_args, n_ignore) \
    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)]; \
    mp_arg_parse_all(n_args - n_ignore, pos_args + n_ignore, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args)

// Parse the arguments of a function
#define PB_PARSE_ARGS_FUNCTION(parsed_args, n_args, pos_args, kw_args, allowed_args) \
    PB_PARSE_ARGS(parsed_args, n_args, pos_args, kw_args, allowed_args, 0)

// Parse the arguments of a class method (like a function, except without parsing self)
#define PB_PARSE_ARGS_METHOD(parsed_args, n_args_including_self, pos_args, kw_args, allowed_args) \
    PB_PARSE_ARGS(parsed_args, n_args_including_self, pos_args, kw_args, allowed_args, 1)

// Required argument
#define PB_ARG_REQUIRED(name) {MP_QSTR_##name, MP_ARG_OBJ | MP_ARG_REQUIRED}

// Optional keyword argument with default integer value
#define PB_ARG_DEFAULT_INT(name, value) {MP_QSTR_##name, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(value)} }

// Optional keyword argument with default false value
#define PB_ARG_DEFAULT_FALSE(name) {MP_QSTR_##name, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_false_obj)} }

// Optional keyword argument with default true value
#define PB_ARG_DEFAULT_TRUE(name) {MP_QSTR_##name, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_true_obj)} }

// Optional keyword argument with default true value
#define PB_ARG_DEFAULT_NONE(name) {MP_QSTR_##name, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} }

// like mp_obj_get_int() but also allows float
#if MICROPY_PY_BUILTINS_FLOAT
mp_int_t pb_obj_get_int(mp_obj_t arg);
#else
#define pb_obj_get_int mp_obj_get_int
#endif

#endif // PYBRICKS_INCLUDED_PBOBJ_H
