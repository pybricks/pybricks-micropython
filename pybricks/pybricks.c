// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objtuple.h"

#include <pybricks/hubs.h>
#include <pybricks/iodevices.h>
#include <pybricks/ev3devices.h>
#include <pybricks/experimental.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/robotics.h>
#include <pybricks/tools.h>

#include "genhdr/mpversion.h"

STATIC const MP_DEFINE_STR_OBJ(pybricks_info_hub_obj, PYBRICKS_HUB_NAME);
STATIC const MP_DEFINE_STR_OBJ(pybricks_info_release_obj, PYBRICKS_RELEASE);
STATIC const MP_DEFINE_STR_OBJ(pybricks_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);

STATIC const mp_rom_obj_tuple_t pybricks_info_obj = {
    {&mp_type_tuple},
    3,
    {
        MP_ROM_PTR(&pybricks_info_hub_obj),
        MP_ROM_PTR(&pybricks_info_release_obj),
        MP_ROM_PTR(&pybricks_info_version_obj),
    }
};

STATIC const mp_rom_map_elem_t pybricks_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pybricks) },
    { MP_ROM_QSTR(MP_QSTR_version),             MP_ROM_PTR(&pybricks_info_obj)},
    #if PYBRICKS_PY_EV3DEVICES
    { MP_ROM_QSTR(MP_QSTR_ev3devices),          MP_ROM_PTR(&pb_module_ev3devices)},
    #endif
    #if PYBRICKS_PY_EXPERIMENTAL
    { MP_ROM_QSTR(MP_QSTR_experimental),        MP_ROM_PTR(&pb_module_experimental)},
    #endif
    #if PYBRICKS_PY_HUBS
    { MP_ROM_QSTR(MP_QSTR_hubs),                MP_ROM_PTR(&pb_module_hubs)        },
    #endif
    #if PYBRICKS_PY_IODEVICES
    { MP_ROM_QSTR(MP_QSTR_iodevices),           MP_ROM_PTR(&pb_module_iodevices)},
    #endif
    #if PYBRICKS_PY_NXTDEVICES
    { MP_ROM_QSTR(MP_QSTR_nxtdevices),          MP_ROM_PTR(&pb_module_nxtdevices)},
    #endif
    #if PYBRICKS_PY_PARAMETERS
    { MP_ROM_QSTR(MP_QSTR_parameters),          MP_ROM_PTR(&pb_module_parameters)  },
    #endif
    #if PYBRICKS_PY_PUPDEVICES
    { MP_ROM_QSTR(MP_QSTR_pupdevices),          MP_ROM_PTR(&pb_module_pupdevices)},
    #endif
    #if PYBRICKS_PY_TOOLS
    { MP_ROM_QSTR(MP_QSTR_tools),               MP_ROM_PTR(&pb_module_tools)  },
    #endif
    #if PYBRICKS_PY_ROBOTICS
    { MP_ROM_QSTR(MP_QSTR_robotics),            MP_ROM_PTR(&pb_module_robotics)},
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_package_pybricks_globals, pybricks_globals_table);

const mp_obj_module_t pb_package_pybricks = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_package_pybricks_globals,
};
