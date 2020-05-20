// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2014-2017 Paul Sokolovsky
// Copyright (c) 2020 The Pybricks Authors

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/persistentcode.h"

#include "genhdr/mpversion.h"

// custom sys.implementation for Pybricks
// See https://docs.python.org/3/library/sys.html#sys.implementation

STATIC const MP_DEFINE_STR_OBJ(pybricks_micropython_obj, "pybricks-micropython");
STATIC const MP_DEFINE_STR_OBJ(pybricks_version_level_obj, PYBRICKS_VERSION_LEVEL_STR);
STATIC const MP_DEFINE_STR_OBJ(git_tag_obj, MICROPY_GIT_TAG);
STATIC const MP_DEFINE_STR_OBJ(build_date_obj, MICROPY_BUILD_DATE);
STATIC const MP_DEFINE_STR_OBJ(hub_name_obj, PYBRICKS_HUB_NAME);

#if MICROPY_PY_ATTRTUPLE
STATIC const qstr pybricks_version_fields[] = {
    MP_QSTR_major, MP_QSTR_minor, MP_QSTR_micro, MP_QSTR_releaselevel, MP_QSTR_serial
};

STATIC MP_DEFINE_ATTRTUPLE(
    pybricks_version_obj,
    pybricks_version_fields,
    5,
    MP_ROM_INT(PYBRICKS_VERSION_MAJOR),
    MP_ROM_INT(PYBRICKS_VERSION_MINOR),
    MP_ROM_INT(PYBRICKS_VERSION_MICRO),
    MP_ROM_PTR(&pybricks_version_level_obj),
    MP_ROM_INT(PYBRICKS_VERSION_SERIAL));

// Standard sys.implementation fields + custom fields (with leading underscore).

STATIC const qstr impl_fields[] = {
    MP_QSTR_name,
    MP_QSTR_version,
    MP_QSTR_hexversion,
    MP_QSTR__git,
    MP_QSTR__date,
    MP_QSTR__mpy,
    MP_QSTR__hub,
};

MP_DEFINE_ATTRTUPLE(
    mp_sys_implementation_obj,
    impl_fields,
    7,
    MP_ROM_PTR(&pybricks_micropython_obj),
    MP_ROM_PTR(&pybricks_version_obj),
    MP_ROM_INT(PYBRICKS_HEXVERSION),
    MP_ROM_PTR(&git_tag_obj),
    MP_ROM_PTR(&build_date_obj),
    MP_ROM_INT(MPY_VERSION),
    MP_ROM_PTR(&hub_name_obj));

#else // MICROPY_PY_ATTRTUPLE

STATIC const mp_rom_obj_tuple_t pybricks_version_obj = {
    {&mp_type_tuple},
    5,
    {
        MP_ROM_INT(PYBRICKS_VERSION_MAJOR),
        MP_ROM_INT(PYBRICKS_VERSION_MINOR),
        MP_ROM_INT(PYBRICKS_VERSION_MICRO),
        MP_ROM_INT(PYBRICKS_VERSION_MICRO),
        MP_ROM_PTR(&pybricks_version_level_obj),
        MP_ROM_INT(PYBRICKS_VERSION_SERIAL),
    }
};

const mp_rom_obj_tuple_t mp_sys_implementation_obj = {
    {&mp_type_tuple},
    7,
    {
        MP_ROM_PTR(&pybricks_micropython_obj),
        MP_ROM_PTR(&pybricks_version_obj),
        MP_ROM_INT(PYBRICKS_HEXVERSION),
        MP_ROM_PTR(&git_tag_obj),
        MP_ROM_PTR(&build_date_obj),
        MP_ROM_INT(MPY_VERSION),
        MP_ROM_PTR(&hub_name_obj),
    }
};

#endif // MICROPY_PY_ATTRTUPLE

#if !MICROPY_PY_SYS

// minimal sys module implementation for small flash devices

STATIC const mp_rom_map_elem_t pb_module_sys_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys) },
    { MP_ROM_QSTR(MP_QSTR_implementation), MP_ROM_PTR(&mp_sys_implementation_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_sys_globals, pb_module_sys_globals_table);

const mp_obj_module_t pb_module_sys = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_sys_globals,
};

#endif // !MICROPY_PY_SYS
