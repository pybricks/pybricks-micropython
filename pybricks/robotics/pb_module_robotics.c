// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS

#include "robotics.h"

STATIC const mp_rom_map_elem_t robotics_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_robotics)   },
    { MP_ROM_QSTR(MP_QSTR_DriveBase),   MP_ROM_PTR(&pb_type_drivebase)  },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_robotics_globals, robotics_globals_table);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_robotics_globals,
};

#endif // PYBRICKS_PY_ROBOTICS
