#include "motordevices.h"

const mp_obj_type_id_t motor_MovehubMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_MovehubMotor,
    .print = motor_DCMotor_print,
    .make_new = motor_DCMotor_make_new,
    .parent = &motor_DCMotor_type,
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
    .device_id = PBIO_ID_PUP_MOVEHUB_MOTOR,
};
