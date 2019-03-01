// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "py/obj.h"
#include <pberror.h>
#include <pbdrv/battery.h>

STATIC mp_obj_t battery_voltage(void) {
    uint16_t volt;
    pb_assert(pbdrv_battery_get_voltage_now(PBIO_PORT_SELF, &volt));
    return mp_obj_new_int(volt);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_voltage_obj, battery_voltage);

STATIC mp_obj_t battery_current(void) {
    uint16_t cur;
    pb_assert(pbdrv_battery_get_current_now(PBIO_PORT_SELF, &cur));
    return mp_obj_new_int(cur);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_current_obj, battery_current);

/* battery module tables */

STATIC const mp_rom_map_elem_t battery_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_battery)        },
    { MP_ROM_QSTR(MP_QSTR_voltage),     MP_ROM_PTR(&battery_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_current),     MP_ROM_PTR(&battery_current_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_battery_globals, battery_globals_table);

const mp_obj_module_t pb_module_battery = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_battery_globals,
};
