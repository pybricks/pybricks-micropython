// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pybricks/parameters.h>
#include <pybricks/parameters/pb_type_button.h>

static const mp_rom_map_elem_t parameters_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_parameters)     },
    { MP_ROM_QSTR(MP_QSTR_Axis),        MP_ROM_PTR(&pb_enum_type_Axis)      },
    #if PYBRICKS_PY_PARAMETERS_BUTTON
    { MP_ROM_QSTR(MP_QSTR_Button),      MP_ROM_PTR(&pb_type_button)    },
    #endif
    { MP_ROM_QSTR(MP_QSTR_Color),       MP_ROM_PTR(&pb_type_Color_obj)      },
    { MP_ROM_QSTR(MP_QSTR_Direction),   MP_ROM_PTR(&pb_enum_type_Direction) },
    #if PYBRICKS_PY_PARAMETERS_ICON
    { MP_ROM_QSTR(MP_QSTR_Icon),        MP_ROM_PTR(&pb_Icon_obj)            },
    #endif
    #if PYBRICKS_PY_PARAMETERS_IMAGE
    { MP_ROM_QSTR(MP_QSTR_Image),       MP_ROM_PTR(&pb_type_Image)          },
    #endif
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&pb_enum_type_Port)      },
    { MP_ROM_QSTR(MP_QSTR_Side),        MP_ROM_PTR(&pb_enum_type_Side)      },
    { MP_ROM_QSTR(MP_QSTR_Stop),        MP_ROM_PTR(&pb_enum_type_Stop)      },
};
static MP_DEFINE_CONST_DICT(pb_module_parameters_globals, parameters_globals_table);

const mp_obj_module_t pb_module_parameters = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_parameters_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_parameters, pb_module_parameters);
#endif

#endif // PYBRICKS_PY_PARAMETERS
