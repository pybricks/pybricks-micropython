#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include <pbio/motorcontrol.h>
#include "modmotor.h"

STATIC mp_obj_t debug_reference(mp_obj_t arg) {
    printf("Debug, world!\n");
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(debug_reference_obj, debug_reference);

const mp_obj_type_id_t motor_FakeDCMotor_type = {
    { &mp_type_type },
    .name = MP_QSTR_FakeMotor,
    .print = motor_DCMotor_print,
    .make_new = motor_DCMotor_make_new,
    .parent = &motor_DCMotor_type,
    .locals_dict = (mp_obj_dict_t*)&motor_DCMotor_locals_dict,
    .device_id = PBIO_ID_UNKNOWN_DCMOTOR,
};

/*
Motor module tables
*/

STATIC const mp_map_elem_t debug_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_debug) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reference), (mp_obj_t)&debug_reference_obj},
    { MP_OBJ_NEW_QSTR(MP_QSTR_FakeMotor), (mp_obj_t)&motor_FakeDCMotor_type},
};

STATIC MP_DEFINE_CONST_DICT (mp_module_debug_globals, debug_globals_table);

const mp_obj_module_t mp_module_debug = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_debug_globals,
};
