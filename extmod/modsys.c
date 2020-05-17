// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2014-2017 Paul Sokolovsky
// Copyright (c) 2020 The Pybricks Authors

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objstr.h"
#include "py/objtuple.h"

#include "genhdr/mpversion.h"

// custom sys.implementation for Pybricks

STATIC const MP_DEFINE_STR_OBJ(pybricks_micropython_obj, "pybricks-micropython");
STATIC const MP_DEFINE_STR_OBJ(pybricks_version_level_obj, PYBRICKS_VERSION_LEVEL_STR);
STATIC const MP_DEFINE_STR_OBJ(git_tag_obj, MICROPY_GIT_TAG);
STATIC const MP_DEFINE_STR_OBJ(build_date_obj, MICROPY_BUILD_DATE);

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
    MP_ROM_INT(PYBRICKS_VERSION_SERIAL)
);

STATIC const qstr impl_fields[] = {
    MP_QSTR_name, MP_QSTR_version, MP_QSTR_hexversion, MP_QSTR__git, MP_QSTR__date
};

MP_DEFINE_ATTRTUPLE(
    mp_sys_implementation_obj,
    impl_fields,
    5,
    MP_ROM_PTR(&pybricks_micropython_obj),
    MP_ROM_PTR(&pybricks_version_obj),
    MP_ROM_INT(PYBRICKS_HEXVERSION),
    MP_ROM_PTR(&git_tag_obj),
    MP_ROM_PTR(&build_date_obj)
);

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
    5,
    {
        MP_ROM_PTR(&pybricks_micropython_obj),
        MP_ROM_PTR(&pybricks_version_obj),
        MP_ROM_INT(PYBRICKS_HEXVERSION),
        MP_ROM_PTR(&git_tag_obj),
        MP_ROM_PTR(&build_date_obj),
    }
};

#endif // MICROPY_PY_ATTRTUPLE
