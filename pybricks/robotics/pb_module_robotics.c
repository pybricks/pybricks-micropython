// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS

#include <pybricks/robotics.h>

STATIC const mp_rom_map_elem_t robotics_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_robotics)   },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_DriveBase),   MP_ROM_PTR(&pb_type_drivebase)  },
    #if (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)
    { MP_ROM_QSTR(MP_QSTR_SpikeBase),   MP_ROM_PTR(&pb_type_spikebase)  },
    #endif
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_robotics_globals, robotics_globals_table);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_robotics_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_robotics, pb_module_robotics);

#endif // PYBRICKS_PY_ROBOTICS
