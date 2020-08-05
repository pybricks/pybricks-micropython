// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/pupdevices.h>

STATIC const mp_rom_map_elem_t pupdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pupdevices)                    },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_Motor),               MP_ROM_PTR(&pb_type_Motor)                         },
    { MP_ROM_QSTR(MP_QSTR_DCMotor),             MP_ROM_PTR(&pb_type_DCMotor)                       },
    #endif
    { MP_ROM_QSTR(MP_QSTR_ColorDistanceSensor), MP_ROM_PTR(&pb_type_pupdevices_ColorDistanceSensor)},
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),         MP_ROM_PTR(&pb_type_pupdevices_ColorSensor)        },
    { MP_ROM_QSTR(MP_QSTR_ForceSensor),         MP_ROM_PTR(&pb_type_pupdevices_ForceSensor)        },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor),      MP_ROM_PTR(&pb_type_pupdevices_InfraredSensor)     },
    { MP_ROM_QSTR(MP_QSTR_Light),               MP_ROM_PTR(&pb_type_pupdevices_Light)              },
    { MP_ROM_QSTR(MP_QSTR_TiltSensor),          MP_ROM_PTR(&pb_type_pupdevices_TiltSensor)         },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor),    MP_ROM_PTR(&pb_type_pupdevices_UltrasonicSensor)   },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_pupdevices_globals, pupdevices_globals_table);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_pupdevices_globals,
};

#endif // PYBRICKS_PY_PUPDEVICES
