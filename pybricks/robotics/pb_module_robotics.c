// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS

#include <pybricks/robotics.h>

#if PYBRICKS_PY_ROBOTICS_EXTRA

#if !MICROPY_MODULE_ATTR_DELEGATION
#error "exra modules requires MICROPY_MODULE_ATTR_DELEGATION"
#endif

#include "py/objmodule.h"
#include "py/builtin.h"
#include "py/runtime.h"

// Need to have an explicit __init__, otherwise MicroPython will try to find
// it in the Python module, which leads to an unnecessary import if none of
// the Python classes are used.
STATIC mp_obj_t pb_module_robotics___init__(void) {
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pb_module_robotics___init___obj, pb_module_robotics___init__);

STATIC void pb_module_robotics_extra_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    const mp_obj_t import_args[] = { MP_OBJ_NEW_QSTR(MP_QSTR__robotics_extra) };
    mp_obj_t module = mp_builtin___import__(MP_ARRAY_SIZE(import_args), import_args);
    mp_load_method_maybe(module, attr, dest);
}
#endif // PYBRICKS_PY_ROBOTICS_EXTRA

STATIC const mp_rom_map_elem_t robotics_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_robotics)   },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_DriveBase),   MP_ROM_PTR(&pb_type_drivebase)  },
    #if PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE
    { MP_ROM_QSTR(MP_QSTR_SpikeBase),   MP_ROM_PTR(&pb_type_spikebase)  },
    #endif
    #endif
    #if PYBRICKS_PY_ROBOTICS_EXTRA
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&pb_module_robotics___init___obj) },
    MP_MODULE_ATTR_DELEGATION_ENTRY(&pb_module_robotics_extra_attr),
    #endif // PYBRICKS_PY_ROBOTICS_EXTRA
};
STATIC MP_DEFINE_CONST_DICT(pb_module_robotics_globals, robotics_globals_table);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_robotics_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_robotics, pb_module_robotics);

#endif // PYBRICKS_PY_ROBOTICS
