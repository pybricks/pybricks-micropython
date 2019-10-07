// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

// LEGO TECHNIC CONTROL+ Hub MicroPython module

#include "py/mpconfig.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "pbhub.h"

extern const struct _mp_obj_module_t pb_module_battery;
extern const struct _mp_obj_module_t pb_module_colorlight;

STATIC const mp_rom_map_elem_t cplushub_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(PYBRICKS_HUB_NAME)      },
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)      },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_PTR(&pb_module_colorlight)   },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hub_shutdown_obj)       },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hub_reboot_obj)         },
    { MP_ROM_QSTR(MP_QSTR_update),      MP_ROM_PTR(&hub_update_obj)         },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_cplushub_globals, cplushub_globals_table);

const mp_obj_module_t pb_module_cplushub = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_cplushub_globals,
};
