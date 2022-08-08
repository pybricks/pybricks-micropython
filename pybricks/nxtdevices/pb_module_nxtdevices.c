// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>

STATIC const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),          MP_ROM_QSTR(MP_QSTR_nxtdevices)                  },
    #if PYBRICKS_PY_COMMON_MOTORS && !PYBRICKS_PY_EV3DEVICES
    { MP_ROM_QSTR(MP_QSTR_Motor),             MP_ROM_PTR(&pb_type_Motor)                       },
    #endif
    #if PYBRICKS_PY_EV3DEVICES
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),       MP_ROM_PTR(&pb_type_nxtdevices_ColorSensor)      },
    { MP_ROM_QSTR(MP_QSTR_EnergyMeter),       MP_ROM_PTR(&pb_type_nxtdevices_EnergyMeter)      },
    { MP_ROM_QSTR(MP_QSTR_LightSensor),       MP_ROM_PTR(&pb_type_nxtdevices_LightSensor)      },
    { MP_ROM_QSTR(MP_QSTR_SoundSensor),       MP_ROM_PTR(&pb_type_nxtdevices_SoundSensor)      },
    { MP_ROM_QSTR(MP_QSTR_TemperatureSensor), MP_ROM_PTR(&pb_type_nxtdevices_TemperatureSensor)},
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),       MP_ROM_PTR(&pb_type_nxtdevices_TouchSensor)      },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor),  MP_ROM_PTR(&pb_type_nxtdevices_UltrasonicSensor) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_nxtdevices_globals,
};

#if PYBRICKS_HUB_EV3BRICK
// ev3dev extends the C module in Python
MP_REGISTER_MODULE(MP_QSTR__nxtdevices, pb_module_nxtdevices);
#else
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_nxtdevices, pb_module_nxtdevices);
#endif

#endif // PYBRICKS_PY_NXTDEVICES
