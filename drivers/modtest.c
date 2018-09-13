#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>
#include "motordevices.h"

STATIC const mp_map_elem_t test_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_test) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_MovehubMotor), (mp_obj_t)&motor_MovehubMotor_type},
};

STATIC MP_DEFINE_CONST_DICT (mp_module_test_globals, test_globals_table);

const mp_obj_module_t mp_module_test = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_test_globals,
};
