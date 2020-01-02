// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#ifndef PYBRICKS_INCLUDED_PBKWARG_H
#define PYBRICKS_INCLUDED_PBKWARG_H

#include "py/obj.h"

// The following macro is a direct copy of https://stackoverflow.com/a/50371430/11744630
#define EXPAND(x) x
#define _GET_NTH_ARG(_1, _2, _3, _4, _5, _6, _7, _9, _10, N, ...) N
#define NUM_ARGS(...) EXPAND(_GET_NTH_ARG(__VA_ARGS__,9,8,7,6,5,4,3,2,1,0))

// Perform an action on a variable number of arguments. Extended from https://stackoverflow.com/a/11994395/11744630
#define FI_1(WHAT, N, X)      WHAT(N-1, X)
#define FI_2(WHAT, N, X, ...) WHAT(N-2, X)FI_1(WHAT, N, __VA_ARGS__)
#define FI_3(WHAT, N, X, ...) WHAT(N-3, X)FI_2(WHAT, N, __VA_ARGS__)
#define FI_4(WHAT, N, X, ...) WHAT(N-4, X)FI_3(WHAT, N, __VA_ARGS__)
#define FI_5(WHAT, N, X, ...) WHAT(N-5, X)FI_4(WHAT, N, __VA_ARGS__)
#define FI_6(WHAT, N, X, ...) WHAT(N-6, X)FI_5(WHAT, N, __VA_ARGS__)
#define FI_7(WHAT, N, X, ...) WHAT(N-7, X)FI_6(WHAT, N, __VA_ARGS__)
#define FI_8(WHAT, N, X, ...) WHAT(N-8, X)FI_7(WHAT, N, __VA_ARGS__)
#define FI_9(WHAT, N, X, ...) WHAT(N-9, X)FI_8(WHAT, N, __VA_ARGS__)
#define GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,NAME,...) NAME
#define FOR_EACH_IDX(action,...) \
  GET_MACRO(__VA_ARGS__,FI_9,FI_8,FI_7,FI_6,FI_5,FI_4,FI_3,FI_2,FI_1,)(action, NUM_ARGS(__VA_ARGS__), __VA_ARGS__)

// Make a QSTR, even if the name is generated from a macro
#define MAKE_QSTR_(name) MP_QSTR_##name
#define MAKE_QSTR(name) MAKE_QSTR_(name)

// Parse given positional and keyword arguments against a list of allowed arguments
// First n_ignore arguments are required arguments for which no keyword can be given.
#define PB_PARSE_ARGS(parsed_args, n_args, pos_args, kw_args, allowed_args, n_ignore) \
    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)]; \
    mp_arg_parse_all(n_args - n_ignore, pos_args + n_ignore, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args)

// The following functions make use of the aforementioned PB_PARSE_ARGS macro, but they first
// auto-generate the allowed_args table to simplify notation in the pybricks modules.

// Generate table entry from an argument name, its requirements, and its default value
#define GET_ARG_NAME(name, required, value) name
#define GET_ARG_SPEC(name, required, value) {MAKE_QSTR(name), required, value}

// Unpack a table entry into three arguments
#define PB_ARG_DO(IDX, arg_spec) GET_ARG_SPEC arg_spec,

// Auto-declare a mp_obj_t for an argument
#define GEN_ARG_OBJ(IDX, arg_spec) mp_obj_t GET_ARG_NAME arg_spec = parsed_args[IDX].u_obj;

// Create the arguments table, parse it, and declare mp_obj_t's for each one
#define PB_PARSE_GENERIC(n_args, pos_args, kw_args, n_ignore, ...) STATIC const mp_arg_t allowed_args[] = { \
        FOR_EACH_IDX(PB_ARG_DO, __VA_ARGS__) \
    }; \
    PB_PARSE_ARGS(parsed_args, n_args, pos_args, kw_args, allowed_args, n_ignore); \
    FOR_EACH_IDX(GEN_ARG_OBJ, __VA_ARGS__)

// Parse the arguments of a function
#define PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args, ...) \
    PB_PARSE_GENERIC(n_args, pos_args, kw_args, 0, __VA_ARGS__)

// Parse the arguments of a class method (like a function, except without parsing self)
#define PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args, self_type, self_name, ...) \
    PB_PARSE_GENERIC(n_args, pos_args, kw_args, 1, __VA_ARGS__) \
    self_type *self_name = MP_OBJ_TO_PTR(pos_args[0]);

// Parse the arguments of a __init__ (without parsing self)
#define PB_PARSE_ARGS_CLASS(n_args, n_kw, pos_and_kw_args, ...) \
    mp_map_t kw_args; \
    mp_map_init_fixed_table(&kw_args, n_kw, pos_and_kw_args + n_args); \
    PB_PARSE_GENERIC(n_args, pos_and_kw_args, &kw_args, 0, __VA_ARGS__)

// Required argument
#define PB_ARG_REQUIRED(name) (name, MP_ARG_OBJ | MP_ARG_REQUIRED, )

// Optional keyword argument with default integer value
#define PB_ARG_DEFAULT_INT(name, value) (name, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_SMALL_INT(value)} )

// Optional keyword argument with default enum value
#define PB_ARG_DEFAULT_ENUM(name, value) (name, MP_ARG_OBJ, {.u_rom_obj = MP_OBJ_FROM_PTR(&value)} )

// Optional keyword argument with default false value
#define PB_ARG_DEFAULT_FALSE(name)(name, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_false_obj)})

// Optional keyword argument with default true value
#define PB_ARG_DEFAULT_TRUE(name)(name, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_true_obj)})

// Optional keyword argument with default true value
#define PB_ARG_DEFAULT_NONE(name)(name, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)})

#endif // PYBRICKS_INCLUDED_PBKWARG_H
