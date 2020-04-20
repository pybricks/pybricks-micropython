// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_MEDIA_EV3DEV

#include "py/obj.h"

#include "pb_ev3dev_types.h"

#if !MICROPY_MODULE_BUILTIN_INIT
#error "media.ev3dev module requires that MICROPY_MODULE_BUILTIN_INIT is enabled"
#endif

STATIC mp_obj_t media_ev3dev___init__() {
    pb_type_ev3dev_Font_init();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(media_ev3dev___init___obj, media_ev3dev___init__);

STATIC const mp_rom_map_elem_t media_ev3dev_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_media_ev3dev)         },
    { MP_ROM_QSTR(MP_QSTR___init__),    MP_ROM_PTR(&media_ev3dev___init___obj)    },
    { MP_ROM_QSTR(MP_QSTR_Font),        MP_ROM_PTR(&pb_type_ev3dev_Font)       },
    { MP_ROM_QSTR(MP_QSTR_Image),       MP_ROM_PTR(&pb_type_ev3dev_Image)      },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_media_ev3dev_globals, media_ev3dev_globals_table);

const mp_obj_module_t pb_module_media_ev3dev = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_media_ev3dev_globals,
};

#endif // PYBRICKS_PY_MEDIA_EV3DEV
