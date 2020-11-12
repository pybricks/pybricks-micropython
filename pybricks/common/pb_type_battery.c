// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include <pbdrv/battery.h>

#include <pbio/button.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/util_pb/pb_error.h>

// TODO: battery is currently a module due to the legacy C API, but should be
// an instance of a Battery type. That would make it consistent with the other
// C types and the high level Python API.

STATIC mp_obj_t battery_voltage(void) {
    uint16_t volt;
    pb_assert(pbdrv_battery_get_voltage_now(&volt));
    return mp_obj_new_int(volt);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_voltage_obj, battery_voltage);

STATIC mp_obj_t battery_current(void) {
    uint16_t cur;
    pb_assert(pbdrv_battery_get_current_now(&cur));
    return mp_obj_new_int(cur);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_current_obj, battery_current);

STATIC mp_obj_t battery_type(void) {
    pbdrv_battery_type_t type;
    pb_assert(pbdrv_battery_get_type(&type));
    switch (type) {
        case PBDRV_BATTERY_TYPE_ALKALINE:
            return MP_OBJ_NEW_QSTR(MP_QSTR_Alkaline);
        case PBDRV_BATTERY_TYPE_LIION:
            return MP_OBJ_NEW_QSTR(MP_QSTR_Li_hyphen_ion);
        default:
            return MP_OBJ_NEW_QSTR(MP_QSTR_Unknown);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_type_obj, battery_type);

/* battery module tables */

STATIC const mp_rom_map_elem_t battery_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_battery)        },
    { MP_ROM_QSTR(MP_QSTR_voltage),     MP_ROM_PTR(&battery_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_current),     MP_ROM_PTR(&battery_current_obj)    },
    { MP_ROM_QSTR(MP_QSTR_type),        MP_ROM_PTR(&battery_type_obj)       },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_battery_globals, battery_globals_table);

const mp_obj_module_t pb_module_battery = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_battery_globals,
};

#endif // PYBRICKS_PY_COMMON
