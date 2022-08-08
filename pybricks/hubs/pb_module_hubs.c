// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS

#include "py/obj.h"
#include "py/runtime.h"

#include <pbdrv/config.h>
#include <pybricks/hubs.h>

STATIC const mp_rom_map_elem_t hubs_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),              MP_ROM_QSTR(MP_QSTR_hubs)    },
    { MP_ROM_QSTR(MP_QSTR_ThisHub),               MP_ROM_PTR(&pb_type_ThisHub) },
    { MP_ROM_QSTR(PYBRICKS_HUB_CLASS_NAME),       MP_ROM_PTR(&pb_type_ThisHub) },
    #ifdef PYBRICKS_HUB_CLASS_NAME_ALIAS
    { MP_ROM_QSTR(PYBRICKS_HUB_CLASS_NAME_ALIAS), MP_ROM_PTR(&pb_type_ThisHub) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_hubs_globals, hubs_globals_table);

const mp_obj_module_t pb_module_hubs = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_hubs_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_hubs, pb_module_hubs);

#endif // PYBRICKS_PY_HUBS
