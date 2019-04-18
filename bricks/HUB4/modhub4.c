// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include "extmod/utime_mphal.h"
#include "py/mperrno.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "modmotor.h"
#include "modhubcommon.h"
#include "modcommon.h"
#include "pberror.h"
#include "pbobj.h"

#include "mpconfigbrick.h"


/* Move Hub module table */

extern const struct _mp_obj_module_t pb_module_battery;

STATIC const mp_rom_map_elem_t hub4_globals_table[] = {
    /* Unique to Move Hub */
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_hub4)    },
    /* Common to Powered Up hubs */
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)  },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hub_shutdown_obj)   },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hub_reboot_obj)     },
    { MP_ROM_QSTR(MP_QSTR_update),      MP_ROM_PTR(&hub_update_obj)     },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_PTR(&hub_set_light_obj)  },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_hub4_globals, hub4_globals_table);

const mp_obj_module_t pb_module_hub4 = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_hub4_globals,
};
