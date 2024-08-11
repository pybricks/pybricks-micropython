// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbdrv/legodev.h>
#include <pbdrv/legodev.h>

#include <pybricks/common.h>
#include <pybricks/pupdevices.h>

static const mp_rom_map_elem_t pupdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pupdevices)                    },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_Motor),               MP_ROM_PTR(&pb_type_Motor)                         },
    { MP_ROM_QSTR(MP_QSTR_DCMotor),             MP_ROM_PTR(&pb_type_DCMotor)                       },
    #endif
    { MP_ROM_QSTR(MP_QSTR_ColorDistanceSensor), MP_ROM_PTR(&pb_type_pupdevices_ColorDistanceSensor)},
    { MP_ROM_QSTR(MP_QSTR_ColorLightMatrix),    MP_ROM_PTR(&pb_type_pupdevices_ColorLightMatrix)   },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),         MP_ROM_PTR(&pb_type_pupdevices_ColorSensor)        },
    { MP_ROM_QSTR(MP_QSTR_ForceSensor),         MP_ROM_PTR(&pb_type_pupdevices_ForceSensor)        },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor),      MP_ROM_PTR(&pb_type_pupdevices_InfraredSensor)     },
    { MP_ROM_QSTR(MP_QSTR_Light),               MP_ROM_PTR(&pb_type_pupdevices_Light)              },
    { MP_ROM_QSTR(MP_QSTR_PFMotor),             MP_ROM_PTR(&pb_type_pupdevices_PFMotor)            },
    #if PYBRICKS_PY_PUPDEVICES_REMOTE
    { MP_ROM_QSTR(MP_QSTR_Remote),              MP_ROM_PTR(&pb_type_pupdevices_Remote)             },
    #endif
    { MP_ROM_QSTR(MP_QSTR_TiltSensor),          MP_ROM_PTR(&pb_type_pupdevices_TiltSensor)         },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor),    MP_ROM_PTR(&pb_type_pupdevices_UltrasonicSensor)   },
};
static MP_DEFINE_CONST_DICT(pb_module_pupdevices_globals, pupdevices_globals_table);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_pupdevices_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_pupdevices, pb_module_pupdevices);

#endif // PYBRICKS_PY_PUPDEVICES
