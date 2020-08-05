// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS

#include <pybricks/hubs.h>

STATIC const mp_rom_map_elem_t hubs_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_hubs)      },
    #if PYBRICKS_HUB_MOVEHUB
    { MP_ROM_QSTR(MP_QSTR_MoveHub),    MP_ROM_PTR(&pb_type_MoveHub) },
    #endif
    #if PYBRICKS_HUB_CITYHUB
    { MP_ROM_QSTR(MP_QSTR_CityHub),    MP_ROM_PTR(&pb_type_CityHub) },
    #endif
    #if PYBRICKS_HUB_CPLUSHUB
    { MP_ROM_QSTR(MP_QSTR_CPlusHub),    MP_ROM_PTR(&pb_type_CPlusHub) },
    #endif
    #ifdef PYBRICKS_HUB_PRIMEHUB
    { MP_ROM_QSTR(MP_QSTR_PrimeHub),    MP_ROM_PTR(&pb_type_PrimeHub) },
    #endif
    #if PYBRICKS_HUB_NXTBRICK
    { MP_ROM_QSTR(MP_QSTR_NXTBrick),    MP_ROM_PTR(&pb_type_NXTBrick) },
    #endif
    #if PYBRICKS_HUB_EV3BRICK
    { MP_ROM_QSTR(MP_QSTR_EV3Brick),    MP_ROM_PTR(&pb_type_EV3Brick) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_hubs_globals, hubs_globals_table);

const mp_obj_module_t pb_module_hubs = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_hubs_globals,
};

#endif // PYBRICKS_PY_HUBS
