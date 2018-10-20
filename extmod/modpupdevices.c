#include <modmotor.h>

#if PBIO_CONFIG_ENABLE_MOTORS
const mp_obj_type_id_t motor_InteractiveMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_InteractiveMotor,
    .device_id = PBIO_ID_PUP_INTERACTIVE_MOTOR,
    .print = motor_EncodedMotor_print,
    .make_new = motor_EncodedMotor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_EncodedMotor_locals_dict,
};
#endif //PBIO_CONFIG_ENABLE_MOTORS


STATIC const mp_map_elem_t pupdevices_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_devices) },
#if PBIO_CONFIG_ENABLE_MOTORS
    { MP_OBJ_NEW_QSTR(MP_QSTR_NORMAL),     MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_DIR_NORMAL  ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_INVERTED),   MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_DIR_INVERTED) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_COAST),      MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_STOP_COAST  ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_BRAKE),      MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_STOP_BRAKE  ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_HOLD),       MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_STOP_HOLD   ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_SMOOTH),     MP_OBJ_NEW_SMALL_INT(1) }, // TODO: Define in PBIO
    { MP_OBJ_NEW_QSTR(MP_QSTR_FAST),       MP_OBJ_NEW_SMALL_INT(0) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_WAIT),       MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_WAIT_COMPLETION ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_BACKGROUND), MP_OBJ_NEW_SMALL_INT(PBIO_MOTOR_WAIT_NONE ) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_InteractiveMotor), (mp_obj_t)&motor_InteractiveMotor_type},
#endif //PBIO_CONFIG_ENABLE_MOTORS
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_pupdevices_globals,
    pupdevices_globals_table
);

const mp_obj_module_t mp_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_pupdevices_globals,
};
