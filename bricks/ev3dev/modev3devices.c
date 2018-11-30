/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Laurens Valk
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_ev3devices_c)   },
    { MP_ROM_QSTR(MP_QSTR_Motor),       MP_ROM_PTR(&motor_Motor_type)       },
    { MP_ROM_QSTR(MP_QSTR_Stop),        MP_ROM_PTR(&motor_Stop_enum)        },
    { MP_ROM_QSTR(MP_QSTR_Dir),         MP_ROM_PTR(&motor_Dir_enum)         },
    { MP_ROM_QSTR(MP_QSTR_Run),         MP_ROM_PTR(&motor_Run_enum)         },
    { MP_ROM_QSTR(MP_QSTR_Color),       MP_ROM_PTR(&pb_Color_enum)          },
    { MP_ROM_QSTR(MP_QSTR_Button),      MP_ROM_PTR(&pb_Button_enum)         },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_ev3devices_globals, ev3devices_globals_table);
const mp_obj_module_t pb_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3devices_globals,
};
