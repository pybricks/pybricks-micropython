// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_SYSTEM

#include "py/obj.h"

#include <pybricks/common.h>

#include <pybricks/util_pb/pb_error.h>

// pybricks.common.System class object
typedef struct _pb_type_System_obj_t {
    mp_obj_base_t base;
} pb_type_System_obj_t;

// dir(pybricks.common.System)
STATIC const mp_rom_map_elem_t common_System_locals_dict_table[] = {
};
STATIC MP_DEFINE_CONST_DICT(common_System_locals_dict, common_System_locals_dict_table);

// type(pybricks.common.System)
STATIC const mp_obj_type_t pb_type_System = {
    { &mp_type_type },
    .name = MP_QSTR_System,
    .locals_dict = (mp_obj_dict_t *)&common_System_locals_dict,
};

// Preinstantiated constant singleton
const mp_obj_base_t pb_type_System_obj = {
    &pb_type_System
};


#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_SYSTEM
