// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include "py/mpconfig.h"

#include "modmotor.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "pbobj.h"
#include "pbkwarg.h"

#include "py/objtype.h"

#include <pberror.h>

// dir(pybricks.nxtdevices)
STATIC const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_nxtdevices)              },
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&motor_Motor_type)                },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_nxtdevices_globals,
};

