// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/builtin.h"
#include "py/mphal.h"
#include "py/objmodule.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>
#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_matrix.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// Implementation of wait that always blocks. Needed for system runloop code
// to briefly wait inside runloop.
STATIC mp_obj_t pb_module_tools__wait_block(mp_obj_t time_in) {
    mp_int_t time = pb_obj_get_int(time_in);
    if (time > 0) {
        mp_hal_delay_ms(time);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_module_tools__wait_block_obj, pb_module_tools__wait_block);

STATIC mp_obj_t tools_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(time));

    // Inside run loop, return generator to await time.
    if (pb_module_tools_run_loop_is_active()) {
        return pb_type_tools_await_time(time_in);
    }

    // Outside of run loop, just block to wait.
    return pb_module_tools__wait_block(time_in);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_wait_obj, 0, tools_wait);

STATIC bool _pb_module_tools_run_loop_is_active;

bool pb_module_tools_run_loop_is_active() {
    return _pb_module_tools_run_loop_is_active;
}

STATIC mp_obj_t pb_module_tools___init__(void) {
    _pb_module_tools_run_loop_is_active = false;
    pb_type_tools_awaitable_init();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(pb_module_tools___init___obj, pb_module_tools___init__);

STATIC mp_obj_t pb_module_tools__set_run_loop_active(mp_obj_t self_in) {
    _pb_module_tools_run_loop_is_active = mp_obj_is_true(self_in);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_module_tools__set_run_loop_active_obj, pb_module_tools__set_run_loop_active);

#if MICROPY_MODULE_ATTR_DELEGATION
// pybricks.tools.task is implemented as pure Python code in the frozen _task
// module. This handler makes it available through the pybricks package.
STATIC void pb_module_tools_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    if (attr == MP_QSTR_task) {
        const mp_obj_t args[] = { MP_OBJ_NEW_QSTR(MP_QSTR__task) };
        dest[0] = mp_builtin___import__(MP_ARRAY_SIZE(args), args);
    }
}
#endif

STATIC const mp_rom_map_elem_t tools_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_tools)      },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&pb_module_tools___init___obj)},
    { MP_ROM_QSTR(MP_QSTR__set_run_loop_active), MP_ROM_PTR(&pb_module_tools__set_run_loop_active_obj)},
    { MP_ROM_QSTR(MP_QSTR__wait_block), MP_ROM_PTR(&pb_module_tools__wait_block_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait),        MP_ROM_PTR(&tools_wait_obj)     },
    { MP_ROM_QSTR(MP_QSTR_StopWatch),   MP_ROM_PTR(&pb_type_StopWatch)  },
    #if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_Matrix),      MP_ROM_PTR(&pb_type_Matrix)           },
    { MP_ROM_QSTR(MP_QSTR_vector),      MP_ROM_PTR(&pb_geometry_vector_obj)   },
    { MP_ROM_QSTR(MP_QSTR_cross),       MP_ROM_PTR(&pb_type_matrix_cross_obj) },
    // backwards compatibility for pybricks.geometry.Axis
    { MP_ROM_QSTR(MP_QSTR_Axis),        MP_ROM_PTR(&pb_enum_type_Axis) },
    #endif // MICROPY_PY_BUILTINS_FLOAT
    #if MICROPY_MODULE_ATTR_DELEGATION
    MP_MODULE_ATTR_DELEGATION_ENTRY(&pb_module_tools_attr),
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_tools_globals, tools_globals_table);

const mp_obj_module_t pb_module_tools = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_tools_globals,
};

#if PYBRICKS_RUNS_ON_EV3DEV
// ev3dev extends the C module in Python
MP_REGISTER_MODULE(MP_QSTR__tools, pb_module_tools);
#else
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_tools, pb_module_tools);
#endif

// backwards compatibility for pybricks.geometry
#if MICROPY_PY_BUILTINS_FLOAT
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_geometry, pb_module_tools);
#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // PYBRICKS_PY_TOOLS
