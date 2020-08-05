// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include "py/runtime.h"

#include "common/common_motors.h"
#include "iodevices/iodevices.h"

STATIC const mp_rom_map_elem_t iodevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_iodevices)                },
    { MP_ROM_QSTR(MP_QSTR_LUMPDevice),       MP_ROM_PTR(&pb_type_iodevices_LUMPDevice)     },
    #if PYBRICKS_PY_EV3DEVICES
    { MP_ROM_QSTR(MP_QSTR_AnalogSensor),     MP_ROM_PTR(&pb_type_iodevices_AnalogSensor)   },
    { MP_ROM_QSTR(MP_QSTR_Ev3devSensor),     MP_ROM_PTR(&pb_type_iodevices_Ev3devSensor)   },
    { MP_ROM_QSTR(MP_QSTR_I2CDevice),        MP_ROM_PTR(&pb_type_iodevices_I2CDevice)      },
    { MP_ROM_QSTR(MP_QSTR_UARTDevice),       MP_ROM_PTR(&pb_type_iodevices_UARTDevice)     },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_DCMotor),          MP_ROM_PTR(&pb_type_DCMotor)                  },
    #endif
    #endif // PYBRICKS_PY_EV3DEVICES
};
STATIC MP_DEFINE_CONST_DICT(pb_module_iodevices_globals, iodevices_globals_table);

const mp_obj_module_t pb_module_iodevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_iodevices_globals,
};

#endif // PYBRICKS_PY_IODEVICES
