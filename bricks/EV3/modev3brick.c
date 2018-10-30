#include <pbio/port.h>
#include <pbobj.h>

/* Port enum */

STATIC const mp_rom_map_elem_t motor_Port_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_A),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_A) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_B),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_B) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_C),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_C) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_D),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_D) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S1),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_1) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S2),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_2) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S3),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_3) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_S4),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_4) },    
};
STATIC PB_DEFINE_CONST_ENUM(motor_Port_enum, motor_Port_enum_table);

/* Module table */

STATIC const mp_map_elem_t ev3brick_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ev3brick_c)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Port), (mp_obj_t)&motor_Port_enum },     
};
 STATIC MP_DEFINE_CONST_DICT (pb_module_ev3brick_globals, ev3brick_globals_table);
 const mp_obj_module_t pb_module_ev3brick = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3brick_globals,
};
