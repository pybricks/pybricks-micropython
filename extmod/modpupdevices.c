#include <modmotor.h>

#if PBIO_CONFIG_ENABLE_MOTORS
const mp_obj_type_t motor_InteractiveMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_InteractiveMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};
const mp_obj_type_t motor_TrainMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_TrainMotor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
};
#endif //PBIO_CONFIG_ENABLE_MOTORS


STATIC const mp_map_elem_t pupdevices_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_devices) },
#if PBIO_CONFIG_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_InteractiveMotor), (mp_obj_t)&motor_InteractiveMotor_type },
    { MP_OBJ_NEW_QSTR(MP_QSTR_TrainMotor),       (mp_obj_t)&motor_TrainMotor_type       },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Stop),             (mp_obj_t)&motor_Stop_enum             },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Dir),              (mp_obj_t)&motor_Dir_enum              },
    { MP_OBJ_NEW_QSTR(MP_QSTR_Run),              (mp_obj_t)&motor_Run_enum              },
#endif //PBIO_CONFIG_ENABLE_MOTORS
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_pupdevices_globals,
    pupdevices_globals_table
);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_pupdevices_globals,
};
