// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbio/port.h>
#include <pbio/button.h>

#include "pberror.h"
#include "pbobj.h"


/* Module table */

extern const struct _mp_obj_module_t pb_module_buttons;
extern const struct _mp_obj_module_t pb_module_battery;
extern const struct _mp_obj_module_t pb_module_colorlight;

STATIC const mp_rom_map_elem_t ev3brick_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_ev3brick )      },
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_PTR(&pb_module_buttons)      },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_PTR(&pb_module_colorlight)   }, 
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)      },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_ev3brick_globals, ev3brick_globals_table);

const mp_obj_module_t pb_module_ev3brick = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3brick_globals,
};
