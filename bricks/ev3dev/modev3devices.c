// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "py/obj.h"
#include "modmotor.h"
#include "modcommon.h"

/*
LEGO MINDSTORMS EV3 Medium Motor.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45502: Separate part (2013)

LEGO ID: 99455/6148292

Compatible with:
Pybricks for LEGO MINDSTORMS NXT
Pybricks for LEGO MINDSTORMS EV3
*/

/*
LEGO MINDSTORMS EV3 Large Motor.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45502: Separate part (2013)

LEGO ID: 95658/6148278

Compatible with:
Pybricks for LEGO MINDSTORMS NXT
Pybricks for LEGO MINDSTORMS EV3
*/

STATIC const mp_rom_map_elem_t ev3devices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_ev3devices)     },
    { MP_ROM_QSTR(MP_QSTR_Motor),       MP_ROM_PTR(&motor_Motor_type)       },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_ev3devices_globals, ev3devices_globals_table);
const mp_obj_module_t pb_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3devices_globals,
};
