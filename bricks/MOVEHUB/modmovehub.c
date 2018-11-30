/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 David Lechner
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

/* Move Hub ports */

STATIC const mp_rom_map_elem_t movehub_Port_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_A),   MP_ROM_INT(PBIO_PORT_A) },
    { MP_ROM_QSTR(MP_QSTR_B),   MP_ROM_INT(PBIO_PORT_B) },
    { MP_ROM_QSTR(MP_QSTR_C),   MP_ROM_INT(PBIO_PORT_C) },
    { MP_ROM_QSTR(MP_QSTR_D),   MP_ROM_INT(PBIO_PORT_D) },
};
STATIC PB_DEFINE_CONST_ENUM(movehub_Port_enum, movehub_Port_enum_table);

/* Move Hub module table */

extern const struct _mp_obj_module_t pb_module_battery;

STATIC const mp_rom_map_elem_t movehub_globals_table[] = {
    /* Unique to Move Hub */
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_movehub)    },
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&movehub_Port_enum)  },
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
