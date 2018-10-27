#include <modmotor.h>
#include <mpconfigbrick.h>
#include <modhubcommon.h>

/* Movehub builtin motors */

#if PBIO_CONFIG_ENABLE_MOTORS
const mp_obj_type_id_t motor_MovehubMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_MovehubMotor,
    .device_id = PBIO_ID_PUP_MOVEHUB_MOTOR,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
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

STATIC const mp_map_elem_t movehub_globals_table[] = {
    /* Unique to Movehub */
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_movehub) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Port), (mp_obj_t)&movehub_Port_enum },
#if PBIO_CONFIG_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_MovehubMotor), (mp_obj_t)&motor_MovehubMotor_type},
#endif //PBIO_CONFIG_ENABLE_MOTORS    
    /* Common to Powered Up hubs */
    { MP_OBJ_NEW_QSTR(MP_QSTR_batt_volt), (mp_obj_t)&hub_batt_volt_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_batt_cur), (mp_obj_t)&hub_batt_cur_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_power_off), (mp_obj_t)&hub_power_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reboot), (mp_obj_t)&hub_reboot_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_light), (mp_obj_t)&hub_set_light_obj },
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
