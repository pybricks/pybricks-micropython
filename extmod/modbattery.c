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
#include <pberror.h>
#include <pbdrv/battery.h>

/*
battery
    def voltage():
        """Return battery voltage (mV: millivolt)."""
*/
STATIC mp_obj_t battery_voltage(void) {
    uint16_t volt;
    pb_assert(pbdrv_battery_get_voltage_now(PBIO_PORT_SELF, &volt));
    return mp_obj_new_int(volt);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_voltage_obj, battery_voltage);

/*
battery
    def current():
        """Return battery current (mA: milliampere)."""
*/
STATIC mp_obj_t battery_current(void) {
    uint16_t cur;
    pb_assert(pbdrv_battery_get_current_now(PBIO_PORT_SELF, &cur));
    return mp_obj_new_int(cur);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_current_obj, battery_current);

/*
battery
    def percent():
        """Return battery percentage between 0% (empty) and 100% (full)."""
*/
STATIC mp_obj_t battery_percent(void) {
    return mp_obj_new_int(100); // TODO
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_percent_obj, battery_percent);

/*
battery
    def low():
        """Return True if the battery is low and False otherwise. Low batteries should be replaced or charged."""
*/
STATIC mp_obj_t battery_low(void) {
    // TODO: return True if voltage too low, false otherwise.
    return mp_obj_new_bool(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(battery_low_obj, battery_low);

/* battery module tables */

STATIC const mp_rom_map_elem_t battery_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_battery)        },
    { MP_ROM_QSTR(MP_QSTR_voltage),     MP_ROM_PTR(&battery_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_current),     MP_ROM_PTR(&battery_current_obj)    },
    { MP_ROM_QSTR(MP_QSTR_percent),     MP_ROM_PTR(&battery_percent_obj)    },
    { MP_ROM_QSTR(MP_QSTR_low),         MP_ROM_PTR(&battery_low_obj)        },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_battery_globals, battery_globals_table);

const mp_obj_module_t pb_module_battery = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_battery_globals,
};
