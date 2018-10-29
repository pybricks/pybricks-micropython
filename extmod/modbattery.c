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
MP_DEFINE_CONST_FUN_OBJ_0(battery_voltage_obj, battery_voltage);

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
MP_DEFINE_CONST_FUN_OBJ_0(battery_current_obj, battery_current);

/*
battery
    def percent():
        """Return battery percentage between 0% (empty) and 100% (full)."""
*/
STATIC mp_obj_t battery_percent(void) {
    return mp_obj_new_int(100); // TODO
}
MP_DEFINE_CONST_FUN_OBJ_0(battery_percent_obj, battery_percent);

/*
battery
    def low():
        """Return True if the battery is low and False otherwise. Low batteries should be replaced or charged."""
*/
STATIC mp_obj_t battery_low(void) {
    // TODO: return True if voltage too low, false otherwise.
    return mp_obj_new_bool(0);
}
MP_DEFINE_CONST_FUN_OBJ_0(battery_low_obj, battery_low);

/* battery module tables */

STATIC const mp_map_elem_t battery_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_battery) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_voltage), (mp_obj_t)&battery_voltage_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_current), (mp_obj_t)&battery_current_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_percent), (mp_obj_t)&battery_percent_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_low    ), (mp_obj_t)&battery_low_obj     },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_battery_globals, battery_globals_table);

const mp_obj_module_t pb_module_battery = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_battery_globals,
};
