// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include <pybricks/common.h>
#include <pybricks/nxtdevices/nxtdevices.h>

static const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),          MP_ROM_QSTR(MP_QSTR_nxtdevices)                  },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_Motor),             MP_ROM_PTR(&pb_type_Motor)                       },
    #endif
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),       MP_ROM_PTR(&pb_type_nxtdevices_TouchSensor)      },
    { MP_ROM_QSTR(MP_QSTR_LightSensor),       MP_ROM_PTR(&pb_type_nxtdevices_LightSensor)      },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),       MP_ROM_PTR(&pb_type_nxtdevices_ColorSensor)      },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor),  MP_ROM_PTR(&pb_type_nxtdevices_UltrasonicSensor) },
    { MP_ROM_QSTR(MP_QSTR_TemperatureSensor), MP_ROM_PTR(&pb_type_nxtdevices_TemperatureSensor)},
    { MP_ROM_QSTR(MP_QSTR_SoundSensor),       MP_ROM_PTR(&pb_type_nxtdevices_SoundSensor)      },
    { MP_ROM_QSTR(MP_QSTR_EnergyMeter),       MP_ROM_PTR(&pb_type_nxtdevices_EnergyMeter)      },
    { MP_ROM_QSTR(MP_QSTR_VernierAdapter),    MP_ROM_PTR(&pb_type_nxtdevices_VernierAdapter)   },
    #if PYBRICKS_PY_NXT_PUP_ALIAS
    { MP_ROM_QSTR(MP_QSTR_ForceSensor),       MP_ROM_PTR(&pb_type_nxtdevices_TouchSensor)      },
    #endif
};
static MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_nxtdevices_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_nxtdevices, pb_module_nxtdevices);
#if PYBRICKS_PY_NXT_PUP_ALIAS
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_pupdevices, pb_module_nxtdevices);
#endif
#endif


#endif // PYBRICKS_PY_NXTDEVICES
