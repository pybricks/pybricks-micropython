// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_MEDIA

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/media.h>

STATIC const mp_rom_map_elem_t media_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_media)      },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_media_globals, media_globals_table);

const mp_obj_module_t pb_module_media = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_media_globals,
};

#endif // PYBRICKS_PY_MEDIA
