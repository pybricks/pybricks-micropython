#include <modmotor.h>
#include <mpconfigbrick.h>
#include <modhubcommon.h>
#include "extmod/utime_mphal.h"

/* Movehub builtin motors */

#if PBIO_CONFIG_ENABLE_MOTORS
const mp_obj_type_t motor_MovehubMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_MovehubMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};
#endif //PBIO_CONFIG_ENABLE_MOTORS

/* Movehub ports */

STATIC const mp_rom_map_elem_t movehub_Port_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_A),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_A) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_B),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_B) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_C),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_C) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_D) },
};
STATIC PB_DEFINE_CONST_ENUM(movehub_Port_enum, movehub_Port_enum_table);

/* Movehub module table */

extern const struct _mp_obj_module_t pb_module_battery;

STATIC const mp_map_elem_t movehub_globals_table[] = {
    /* Unique to Movehub */
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_movehub) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Port), (mp_obj_t)&movehub_Port_enum },
#if PBIO_CONFIG_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_MovehubMotor), (mp_obj_t)&motor_MovehubMotor_type},
#endif //PBIO_CONFIG_ENABLE_MOTORS    
    /* Common to Powered Up hubs */
    { MP_ROM_QSTR(MP_QSTR_wait), (mp_obj_t)&mp_utime_sleep_ms_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Color), (mp_obj_t)&pup_Color_enum },
    { MP_OBJ_NEW_QSTR(MP_QSTR_battery), (mp_obj_t)&pb_module_battery },
    { MP_OBJ_NEW_QSTR(MP_QSTR_shutdown), (mp_obj_t)&hub_shutdown_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reboot), (mp_obj_t)&hub_reboot_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_update), (mp_obj_t)&hub_update_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_light), (mp_obj_t)&hub_set_light_obj },
#if PYBRICKS_ENABLE_HARDWARE_DEBUG
    { MP_OBJ_NEW_QSTR(MP_QSTR_gpios), (mp_obj_t)&hub_gpios_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_read_adc), (mp_obj_t)&hub_read_adc_obj },
#endif //PYBRICKS_ENABLE_HARDWARE_DEBUG
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_movehub_globals,
    movehub_globals_table
);

const mp_obj_module_t pb_module_movehub = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_movehub_globals,
};
