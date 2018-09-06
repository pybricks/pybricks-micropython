#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include <pbio/motor.h>

STATIC const mp_map_elem_t constants_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR__constants) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_A),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_A) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_B),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_B) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_C),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_C) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_PORT_D),   MP_OBJ_NEW_SMALL_INT(PBIO_PORT_D) },    
};

STATIC MP_DEFINE_CONST_DICT (mp_module_constants_globals, constants_globals_table);

const mp_obj_module_t mp_module_constants = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_constants_globals,
};
