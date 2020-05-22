// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2020 The Pybricks Authors

/// \module os - basic "operating system" services

#include "py/mpconfig.h"

#if PYBRICKS_PY_UOS

#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/persistentcode.h"
#include "py/runtime.h"

#include "genhdr/mpversion.h"

STATIC const MP_DEFINE_STR_OBJ(os_uname_info_sysname_obj, "pybricks");
// TODO: nodename should be user-customizable hub name
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_nodename_obj, PYBRICKS_HUB_NAME);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_release_obj, PYBRICKS_RELEASE);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_machine_obj, PYBRICKS_HUB_NAME);

#if MICROPY_PY_ATTRTUPLE
STATIC const qstr os_uname_info_fields[] = {
    MP_QSTR_sysname, MP_QSTR_nodename,
    MP_QSTR_release, MP_QSTR_version, MP_QSTR_machine
};

STATIC MP_DEFINE_ATTRTUPLE(
    os_uname_info_obj,
    os_uname_info_fields,
    5,
    MP_ROM_PTR(&os_uname_info_sysname_obj),
    MP_ROM_PTR(&os_uname_info_nodename_obj),
    MP_ROM_PTR(&os_uname_info_release_obj),
    MP_ROM_PTR(&os_uname_info_version_obj),
    MP_ROM_PTR(&os_uname_info_machine_obj));
#else // MICROPY_PY_ATTRTUPLE
const mp_rom_obj_tuple_t os_uname_info_obj = {
    {&mp_type_tuple},
    5,
    {
        MP_ROM_PTR(&os_uname_info_sysname_obj),
        MP_ROM_PTR(&os_uname_info_nodename_obj),
        MP_ROM_PTR(&os_uname_info_release_obj),
        MP_ROM_PTR(&os_uname_info_version_obj),
        MP_ROM_PTR(&os_uname_info_machine_obj),
    }
};
#endif // MICROPY_PY_ATTRTUPLE

STATIC mp_obj_t os_uname(void) {
    return MP_OBJ_FROM_PTR(&os_uname_info_obj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname_obj, os_uname);

STATIC const mp_rom_map_elem_t os_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uos) },
    { MP_ROM_QSTR(MP_QSTR_uname), MP_ROM_PTR(&os_uname_obj) },
};

STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t pb_module_uos = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&os_module_globals,
};

#endif // PYBRICKS_PY_UOS
