// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include <pybricks/common.h>
#include "iodevices.h"

static const mp_rom_map_elem_t iodevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_iodevices)                },
    { MP_ROM_QSTR(MP_QSTR_PUPDevice),        MP_ROM_PTR(&pb_type_iodevices_PUPDevice)      },
    { MP_ROM_QSTR(MP_QSTR_UARTDevice),       MP_ROM_PTR(&pb_type_uart_device)              },
    #if PYBRICKS_PY_PUPDEVICES && PYBRICKS_PY_PUPDEVICES_REMOTE
    { MP_ROM_QSTR(MP_QSTR_LWP3Device),       MP_ROM_PTR(&pb_type_iodevices_LWP3Device)     },
    #endif
    #if PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER
    { MP_ROM_QSTR(MP_QSTR_XboxController),   MP_ROM_PTR(&pb_type_iodevices_XboxController) },
    #endif
    #if PYBRICKS_PY_EV3DEVDEVICES
    { MP_ROM_QSTR(MP_QSTR_LUMPDevice),       MP_ROM_PTR(&pb_type_iodevices_PUPDevice)      },
    { MP_ROM_QSTR(MP_QSTR_AnalogSensor),     MP_ROM_PTR(&pb_type_iodevices_AnalogSensor)   },
    { MP_ROM_QSTR(MP_QSTR_Ev3devSensor),     MP_ROM_PTR(&pb_type_iodevices_Ev3devSensor)   },
    { MP_ROM_QSTR(MP_QSTR_I2CDevice),        MP_ROM_PTR(&pb_type_iodevices_I2CDevice)      },
    #if PYBRICKS_PY_COMMON_MOTORS
    { MP_ROM_QSTR(MP_QSTR_DCMotor),          MP_ROM_PTR(&pb_type_DCMotor)                  },
    #endif
    #endif // PYBRICKS_PY_EV3DEVDEVICES
};
static MP_DEFINE_CONST_DICT(pb_module_iodevices_globals, iodevices_globals_table);

const mp_obj_module_t pb_module_iodevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_iodevices_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_iodevices, pb_module_iodevices);
#endif

#endif // PYBRICKS_PY_IODEVICES
