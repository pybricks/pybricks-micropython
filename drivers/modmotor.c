#include "modmotor.h"
int global_test;

STATIC mp_obj_t motor_run_target(size_t n_args, const mp_obj_t *args) {    
    printf("Hello world!\n");
    int target = mp_obj_get_int(args[3]);
    global_test = target;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_run_target_obj, 6, 6, motor_run_target);

STATIC const mp_map_elem_t motor_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_motor) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_run_target), (mp_obj_t)&motor_run_target_obj },
};

STATIC MP_DEFINE_CONST_DICT (
    mp_module_motor_globals,
    motor_globals_table
);

const mp_obj_module_t mp_module_motor = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_motor_globals,
};