// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include "py/mpconfig.h"

#if PYBRICKS_PY_RESOURCES

#include "py/obj.h"

#if PYBRICKS_HUB_EV3
#include <pb_ev3dev_types.h>
#endif

#if !MICROPY_MODULE_BUILTIN_INIT
#error "Pybricks resources module requires that MICROPY_MODULE_BUILTIN_INIT is enabled"
#endif

STATIC mp_obj_t resources___init__() {
#if PYBRICKS_HUB_EV3
    pb_type_ev3dev_Font_init();
#endif
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(resources___init___obj, resources___init__);

STATIC const mp_rom_map_elem_t resources_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_resources)         },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&resources___init___obj)    },
#if PYBRICKS_HUB_EV3
    { MP_ROM_QSTR(MP_QSTR_Font),        MP_ROM_PTR(&pb_type_ev3dev_Font)       },
#endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_resources_globals, resources_globals_table);

const mp_obj_module_t pb_module_resources = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_resources_globals,
};

#endif // PYBRICKS_PY_RESOURCES
