// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pybricks/parameters.h>

STATIC const mp_rom_map_elem_t parameters_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_parameters)     },
    #if PYBRICKS_PY_PARAMETERS_BUTTON
    { MP_ROM_QSTR(MP_QSTR_Button),      MP_ROM_PTR(&pb_enum_type_Button)    },
    #endif
    { MP_ROM_QSTR(MP_QSTR_Color),       MP_ROM_PTR(&pb_type_Color)          },
    { MP_ROM_QSTR(MP_QSTR_Direction),   MP_ROM_PTR(&pb_enum_type_Direction) },
    { MP_ROM_QSTR(MP_QSTR_Icon),        MP_ROM_PTR(&pb_Icon_obj)            },
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&pb_enum_type_Port)      },
    { MP_ROM_QSTR(MP_QSTR_Side),        MP_ROM_PTR(&pb_enum_type_Side)      },
    { MP_ROM_QSTR(MP_QSTR_Stop),        MP_ROM_PTR(&pb_enum_type_Stop)      },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_parameters_globals, parameters_globals_table);

const mp_obj_module_t pb_module_parameters = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_parameters_globals,
};

#endif // PYBRICKS_PY_PARAMETERS
