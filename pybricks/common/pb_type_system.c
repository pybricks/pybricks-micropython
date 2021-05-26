// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_SYSTEM

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/common.h>

#include <pybricks/util_pb/pb_error.h>

// pybricks.common.System class object
typedef struct _pb_type_System_obj_t {
    mp_obj_base_t base;
} pb_type_System_obj_t;

#if PBDRV_CONFIG_RESET

// REVISIT: there should be a pbio_reset() instead of pbdrv_reset() to gracefully
// shut down the hub (e.g. if the power button is pressed or USB is plugged in,
// some hubs will not actually shut down).

#include <pbdrv/reset.h>

STATIC mp_obj_t pb_hubs_Hub_reset(mp_obj_t self_in, mp_obj_t action_in) {
    pbdrv_reset_action_t action = mp_obj_get_int(action_in);

    if (action < PBDRV_RESET_ACTION_RESET || action > PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE) {
        mp_raise_ValueError(NULL);
    }

    pbdrv_reset(action);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pb_hubs_Hub_reset_obj, pb_hubs_Hub_reset);

STATIC mp_obj_t pb_hubs_Hub_reset_reason(mp_obj_t self_in) {
    pbdrv_reset_reason_t reason = pbdrv_reset_get_reason();
    return MP_OBJ_NEW_SMALL_INT(reason);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_hubs_Hub_reset_reason_obj, pb_hubs_Hub_reset_reason);

#endif // PBDRV_CONFIG_RESET

// dir(pybricks.common.System)
STATIC const mp_rom_map_elem_t common_System_locals_dict_table[] = {
    #if PBDRV_CONFIG_RESET
    { MP_ROM_QSTR(MP_QSTR_reset),       MP_ROM_PTR(&pb_hubs_Hub_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_reason),MP_ROM_PTR(&pb_hubs_Hub_reset_reason_obj) },
    #endif // PBDRV_CONFIG_RESET
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
