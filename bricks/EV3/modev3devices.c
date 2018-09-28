#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>
#include <modmotor.h>

const mp_obj_type_id_t motor_EV3MediumMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_MediumMotor,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
    .device_id = PBIO_ID_EV3_MEDIUM_MOTOR,
};

const mp_obj_type_id_t motor_EV3LargeMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_LargeMotor,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
    .device_id = PBIO_ID_EV3_LARGE_MOTOR,
};

STATIC const mp_map_elem_t ev3devices_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ev3devices_c) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MediumMotor), (mp_obj_t)&motor_EV3MediumMotor_type},    
    { MP_OBJ_NEW_QSTR(MP_QSTR_LargeMotor), (mp_obj_t)&motor_EV3LargeMotor_type},
};
 STATIC MP_DEFINE_CONST_DICT (mp_module_ev3devices_globals, ev3devices_globals_table);
 const mp_obj_module_t mp_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_ev3devices_globals,
};
