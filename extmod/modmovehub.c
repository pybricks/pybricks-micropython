// SPDX-License-Identifier: MIT
// Copyright (c) 2018 David Lechner

#include "py/mpconfig.h"

#if PYBRICKS_PY_MOVEHUB

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>

#include "extmod/utime_mphal.h"
#include "py/mperrno.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "modmotor.h"
#include "modcommon.h"
#include "pberror.h"
#include "pbhub.h"
#include "pbobj.h"

/* Move Hub module table */

extern const struct _mp_obj_module_t pb_module_battery;

STATIC const mp_rom_map_elem_t movehub_globals_table[] = {
    /* Unique to Move Hub */
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_movehub)    },
    /* Common to Powered Up hubs */
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)  },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hub_shutdown_obj)   },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hub_reboot_obj)     },
    { MP_ROM_QSTR(MP_QSTR_update),      MP_ROM_PTR(&hub_update_obj)     },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_PTR(&hub_set_light_obj)  },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_movehub_globals, movehub_globals_table);

const mp_obj_module_t pb_module_movehub = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_movehub_globals,
};

#endif // PYBRICKS_PY_MOVEHUB
