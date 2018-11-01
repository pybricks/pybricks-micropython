#include "py/obj.h"

/*
LEGO MINDSTORMS EV3 Medium Motor.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45502: Separate part (2013)

LEGO ID: 99455/6148292

Compatible with:
Pybricks for LEGO MINDSTORMS NXT
Pybricks for LEGO MINDSTORMS EV3
*/
const mp_obj_type_t motor_EV3MediumMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_MediumMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};

/*
LEGO MINDSTORMS EV3 Large Motor.

Contained in set:
31313: LEGO MINDSTORMS EV3 (2013)
45544: LEGO MINDSTORMS Education EV3 Core Set (2013)
45502: Separate part (2013)

LEGO ID: 95658/6148278

Compatible with:
Pybricks for LEGO MINDSTORMS NXT
Pybricks for LEGO MINDSTORMS EV3
*/
const mp_obj_type_t motor_EV3LargeMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_LargeMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};

STATIC const mp_map_elem_t ev3devices_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ev3devices_c)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MediumMotor), (mp_obj_t)&motor_EV3MediumMotor_type },    
    { MP_OBJ_NEW_QSTR(MP_QSTR_LargeMotor),  (mp_obj_t)&motor_EV3LargeMotor_type  },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Stop),        (mp_obj_t)&motor_Stop_enum           },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Dir),         (mp_obj_t)&motor_Dir_enum            },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Run),         (mp_obj_t)&motor_Run_enum            },     
};
 STATIC MP_DEFINE_CONST_DICT (pb_module_ev3devices_globals, ev3devices_globals_table);
 const mp_obj_module_t pb_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3devices_globals,
};
