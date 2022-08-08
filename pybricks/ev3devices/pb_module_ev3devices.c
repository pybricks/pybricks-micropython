// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/ev3devices.h>

STATIC const mp_rom_map_elem_t ev3devices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_ev3devices)              },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&pb_type_Motor)                   },
    #endif
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),      MP_ROM_PTR(&pb_type_ev3devices_TouchSensor)     },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor),   MP_ROM_PTR(&pb_type_ev3devices_InfraredSensor)  },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),      MP_ROM_PTR(&pb_type_ev3devices_ColorSensor)     },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor), MP_ROM_PTR(&pb_type_ev3devices_UltrasonicSensor)},
    { MP_ROM_QSTR(MP_QSTR_GyroSensor),       MP_ROM_PTR(&pb_type_ev3devices_GyroSensor)      },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_ev3devices_globals, ev3devices_globals_table);

const mp_obj_module_t pb_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_ev3devices_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_ev3devices, pb_module_ev3devices);

#endif // PYBRICKS_PY_EV3DEVICES
