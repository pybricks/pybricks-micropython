// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Keyword argument parsing helpers for MicroPython functions and methods.
//
// Usage, at the top of a function that takes (n_args, pos_args, kw_args):
//
//     PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
//         PB_ARG_REQUIRED(foo),
//         PB_ARG_DEFAULT_INT(bar, 100));
//
// This parses the arguments with mp_arg_parse_all() and then declares one
// mp_obj_t variable per argument, named after the argument with an _in
// suffix (foo_in and bar_in above). All arguments are parsed as objects and
// may be given by position or by keyword.
//
// Variants:
//
//  - PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args, ...)
//      For functions defined with MP_DEFINE_CONST_FUN_OBJ_KW.
//  - PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args, type, self, ...)
//      Same, but pos_args[0] is the instance. It is not parsed as an
//      argument; it is declared as `type *self` instead.
//  - PB_PARSE_ARGS_CLASS(n_args, n_kw, args, ...)
//      For make_new functions, where positional and keyword arguments are
//      given as one array.
//
// Argument descriptors (between 1 and 16 per call):
//
//  - PB_ARG_REQUIRED(name)             Required argument.
//  - PB_ARG_DEFAULT_INT(name, value)   Optional, default small int value.
//  - PB_ARG_DEFAULT_OBJ(name, value)   Optional, default &value (a ROM object).
//  - PB_ARG_DEFAULT_QSTR(name, value)  Optional, default string MP_QSTR_value.
//  - PB_ARG_DEFAULT_FALSE(name)        Optional, default False.
//  - PB_ARG_DEFAULT_TRUE(name)         Optional, default True.
//  - PB_ARG_DEFAULT_NONE(name)         Optional, default None.
//
// After a PB_PARSE_ARGS_* call, PB_PARSE_ARGS_METHOD_ALL_NONE() evaluates to
// true if every parsed argument (not counting self) is None.
//
// The macros also leave these locals in scope, which callers may use to parse
// the same arguments again (for example with a shorter table):
//
//  - allowed_args: the static const mp_arg_t table.
//  - parsed_args:  the mp_arg_val_t results.
//  - kw_args:      PB_PARSE_ARGS_CLASS only, the mp_map_t of keyword arguments.

#ifndef PYBRICKS_INCLUDED_PB_KWARG_HELPER_H
#define PYBRICKS_INCLUDED_PB_KWARG_HELPER_H

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/util_mp/pb_obj_helper.h>

// Each descriptor expands to a parenthesized tuple:
//
//     (qstr, variable name, index constant name, flags, default value)
//
// The name is token-pasted right here, so an argument name that happens to
// also be a macro name is never expanded.

#define PB_ARG_REQUIRED(name) \
    (MP_QSTR_##name, name##_in, pb_kwarg_idx_##name, MP_ARG_REQUIRED | MP_ARG_OBJ, MP_ROM_PTR(NULL))

#define PB_ARG_DEFAULT_INT(name, value) \
    (MP_QSTR_##name, name##_in, pb_kwarg_idx_##name, MP_ARG_OBJ, MP_ROM_INT(value))

#define PB_ARG_DEFAULT_OBJ(name, value) \
    (MP_QSTR_##name, name##_in, pb_kwarg_idx_##name, MP_ARG_OBJ, MP_ROM_PTR(&value))

#define PB_ARG_DEFAULT_QSTR(name, value) \
    (MP_QSTR_##name, name##_in, pb_kwarg_idx_##name, MP_ARG_OBJ, MP_ROM_QSTR(MP_QSTR_##value))

#define PB_ARG_DEFAULT_FALSE(name) \
    (MP_QSTR_##name, name##_in, pb_kwarg_idx_##name, MP_ARG_OBJ, MP_ROM_FALSE)

#define PB_ARG_DEFAULT_TRUE(name) \
    (MP_QSTR_##name, name##_in, pb_kwarg_idx_##name, MP_ARG_OBJ, MP_ROM_TRUE)

#define PB_ARG_DEFAULT_NONE(name) \
    (MP_QSTR_##name, name##_in, pb_kwarg_idx_##name, MP_ARG_OBJ, MP_ROM_NONE)

// Per-descriptor expansions. Each takes the tuple elements. The default value
// is taken as the variadic tail, because in some object representations a ROM
// object initializer is a braced list that itself contains commas.

// Enumerator giving the position of the argument in the tables below.
#define PB_KWARG_INDEX(qst, var, idx, flags, ...) idx,
// Entry of the mp_arg_t table. The default of a required argument is never
// read by mp_arg_parse_all(), so it is left as a null object.
#define PB_KWARG_ENTRY(qst, var, idx, flags, ...) { qst, flags, { .u_rom_obj = __VA_ARGS__ } },
// Local variable holding the parsed argument.
#define PB_KWARG_LOCAL(qst, var, idx, flags, ...) mp_obj_t var = parsed_args[idx].u_obj;

// Applies macro m to one tuple: m (a, b, c, d, e) -> m(a, b, c, d, e).
#define PB_KWARG_APPLY(m, tuple) m tuple

// Number of variadic arguments, 1 to 16.
#define PB_KWARG_COUNT(...) \
    PB_KWARG_COUNT_PICK(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define PB_KWARG_COUNT_PICK(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, n, ...) n

#define PB_KWARG_JOIN(a, b) PB_KWARG_JOIN_(a, b)
#define PB_KWARG_JOIN_(a, b) a##b

// Applies macro m to each of the variadic tuples, in order.
#define PB_KWARG_EACH(m, ...) PB_KWARG_JOIN(PB_KWARG_EACH_, PB_KWARG_COUNT(__VA_ARGS__))(m, __VA_ARGS__)
#define PB_KWARG_EACH_1(m, t) PB_KWARG_APPLY(m, t)
#define PB_KWARG_EACH_2(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_1(m, __VA_ARGS__)
#define PB_KWARG_EACH_3(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_2(m, __VA_ARGS__)
#define PB_KWARG_EACH_4(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_3(m, __VA_ARGS__)
#define PB_KWARG_EACH_5(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_4(m, __VA_ARGS__)
#define PB_KWARG_EACH_6(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_5(m, __VA_ARGS__)
#define PB_KWARG_EACH_7(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_6(m, __VA_ARGS__)
#define PB_KWARG_EACH_8(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_7(m, __VA_ARGS__)
#define PB_KWARG_EACH_9(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_8(m, __VA_ARGS__)
#define PB_KWARG_EACH_10(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_9(m, __VA_ARGS__)
#define PB_KWARG_EACH_11(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_10(m, __VA_ARGS__)
#define PB_KWARG_EACH_12(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_11(m, __VA_ARGS__)
#define PB_KWARG_EACH_13(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_12(m, __VA_ARGS__)
#define PB_KWARG_EACH_14(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_13(m, __VA_ARGS__)
#define PB_KWARG_EACH_15(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_14(m, __VA_ARGS__)
#define PB_KWARG_EACH_16(m, t, ...) PB_KWARG_APPLY(m, t) PB_KWARG_EACH_15(m, __VA_ARGS__)

// Declares the index constants, the argument table and the output array.
#define PB_KWARG_DECLARE(...) \
    enum { PB_KWARG_EACH(PB_KWARG_INDEX, __VA_ARGS__) }; \
    static const mp_arg_t allowed_args[] = { PB_KWARG_EACH(PB_KWARG_ENTRY, __VA_ARGS__) }; \
    mp_arg_val_t parsed_args[MP_ARRAY_SIZE(allowed_args)]

#define PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args, ...) \
    PB_KWARG_DECLARE(__VA_ARGS__); \
    mp_arg_parse_all((n_args), (pos_args), (kw_args), \
    MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args); \
    PB_KWARG_EACH(PB_KWARG_LOCAL, __VA_ARGS__)

#define PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args, type, self, ...) \
    type *self = MP_OBJ_TO_PTR((pos_args)[0]); \
    PB_PARSE_ARGS_FUNCTION((n_args) - 1, (pos_args) + 1, (kw_args), __VA_ARGS__)

// Like PB_PARSE_ARGS_METHOD, for a method that does not use its instance:
// the instance is skipped, not converted, and no self variable is declared.
#define PB_PARSE_ARGS_METHOD_SKIP_SELF(n_args, pos_args, kw_args, ...) \
    PB_PARSE_ARGS_FUNCTION((n_args) - 1, (pos_args) + 1, (kw_args), __VA_ARGS__)

// Like mp_arg_parse_all_kw_array(), but with the keyword map kept in scope.
#define PB_PARSE_ARGS_CLASS(n_args, n_kw, args, ...) \
    PB_KWARG_DECLARE(__VA_ARGS__); \
    mp_map_t kw_args; \
    mp_map_init_fixed_table(&kw_args, (n_kw), (args) + (n_args)); \
    mp_arg_parse_all((n_args), (args), &kw_args, \
    MP_ARRAY_SIZE(allowed_args), allowed_args, parsed_args); \
    PB_KWARG_EACH(PB_KWARG_LOCAL, __VA_ARGS__)

#define PB_PARSE_ARGS_METHOD_ALL_NONE() \
    pb_obj_parsed_args_all_none(parsed_args, MP_ARRAY_SIZE(parsed_args))

#endif // PYBRICKS_INCLUDED_PB_KWARG_HELPER_H
