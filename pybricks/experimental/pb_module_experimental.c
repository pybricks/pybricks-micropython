// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include <pbio/util.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/util_pb/pb_error.h>

#include <pybricks/robotics.h>

#if PYBRICKS_PY_COMMON_BTC
extern const mp_obj_module_t pb_module_btc;
#endif

// pybricks.experimental.hello_world
static mp_obj_t experimental_hello_world(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        // Add up to 8 arguments below, all separated by one comma.
        // You can choose from:
        //  PB_ARG_REQUIRED(name), This is a required argument.
        //  PB_ARG_DEFAULT_INT(name, value), Keyword argument with default int value.
        //  PB_ARG_DEFAULT_OBJ(name, value), Keyword argument with default MicroPython object.
        //  PB_ARG_DEFAULT_QSTR(name, value), Keyword argument with default string value, without quotes.
        //  PB_ARG_DEFAULT_FALSE(name), Keyword argument with default False value.
        //  PB_ARG_DEFAULT_TRUE(name), Keyword argument with default True value.
        //  PB_ARG_DEFAULT_NONE(name), Keyword argument with default None value.
        PB_ARG_REQUIRED(foo),
        PB_ARG_DEFAULT_NONE(bar));

    // This function can be used in the following ways:
    // from pybricks.experimental import hello_world
    // y = hello_world(5)
    // y = hello_world(foo=5)
    // y = hello_world(5, 6)
    // y = hello_world(foo=5, bar=6)

    // All input arguments are available as MicroPython objects with _in post-fixed to the name.
    // Use MicroPython functions or Pybricks helper functions to unpack them into C types:
    mp_int_t foo = pb_obj_get_int(foo_in);

    // You can check if an argument is none.
    if (bar_in == mp_const_none) {
        // Example of how to print.
        mp_printf(&mp_plat_print, "Bar was not given. Foo is: %d\n", foo);
    }

    // Example of returning an object. Here we return the square of the input argument.
    return mp_obj_new_int(foo * foo);
}
// See also experimental_globals_table below. This function object is added there to make it importable.
static MP_DEFINE_CONST_FUN_OBJ_KW(experimental_hello_world_obj, 0, experimental_hello_world);

static const mp_rom_map_elem_t experimental_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental) },
    { MP_ROM_QSTR(MP_QSTR_hello_world), MP_ROM_PTR(&experimental_hello_world_obj) },
    #if PYBRICKS_PY_COMMON_BTC && MICROPY_MODULE_BUILTIN_SUBPACKAGES
    { MP_ROM_QSTR(MP_QSTR_btc), MP_ROM_PTR(&pb_module_btc) },
    #endif
};
static MP_DEFINE_CONST_DICT(pb_module_experimental_globals, experimental_globals_table);

const mp_obj_module_t pb_module_experimental = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_experimental_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_experimental, pb_module_experimental);
#endif

#endif // PYBRICKS_PY_EXPERIMENTAL
