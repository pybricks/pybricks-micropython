// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "tools/tools.h"

#include "py/mpconfig.h"
#include "py/obj.h"

STATIC const mp_rom_map_elem_t pybricks_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pybricks) },
    #if PYBRICKS_PY_TOOLS
    { MP_ROM_QSTR(MP_QSTR_tools),               MP_ROM_PTR(&pb_module_tools)  },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_package_pybricks_globals, pybricks_globals_table);

const mp_obj_module_t pb_package_pybricks = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_package_pybricks_globals,
};
