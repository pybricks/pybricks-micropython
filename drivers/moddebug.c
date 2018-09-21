#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include <pbio/motorcontrol.h>

STATIC mp_obj_t debug_reference(mp_obj_t arg) {
    printf("Debug, world!\n");
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(debug_reference_obj, debug_reference);

/*
Motor module tables
*/

STATIC const mp_map_elem_t debug_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_debug) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reference), (mp_obj_t)&debug_reference_obj},
};

STATIC MP_DEFINE_CONST_DICT (mp_module_debug_globals, debug_globals_table);

const mp_obj_module_t mp_module_debug = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_debug_globals,
};
