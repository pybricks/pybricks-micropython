// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_matrix.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

STATIC mp_obj_t tools_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(time));

    mp_int_t time = pb_obj_get_int(time_in);
    if (time > 0) {
        mp_hal_delay_ms(time);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(tools_wait_obj, 0, tools_wait);

STATIC const mp_rom_map_elem_t tools_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_tools)      },
    { MP_ROM_QSTR(MP_QSTR_wait),        MP_ROM_PTR(&tools_wait_obj)     },
    { MP_ROM_QSTR(MP_QSTR_StopWatch),   MP_ROM_PTR(&pb_type_StopWatch)  },
    #if MICROPY_PY_BUILTINS_FLOAT
    { MP_ROM_QSTR(MP_QSTR_Matrix),      MP_ROM_PTR(&pb_type_Matrix)           },
    { MP_ROM_QSTR(MP_QSTR_vector),      MP_ROM_PTR(&pb_geometry_vector_obj)   },
    { MP_ROM_QSTR(MP_QSTR_cross),       MP_ROM_PTR(&pb_type_matrix_cross_obj) },
    // backwards compatibility for pybricks.geometry.Axis
    { MP_ROM_QSTR(MP_QSTR_Axis),        MP_ROM_PTR(&pb_enum_type_Axis) },
    #endif // MICROPY_PY_BUILTINS_FLOAT
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
