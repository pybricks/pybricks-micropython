// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS

#include "py/obj.h"
#include "py/runtime.h"

#include <pbdrv/config.h>
#include <pybricks/hubs.h>

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
MP_DEFINE_CONST_FUN_OBJ_2(pb_hubs_Hub_reset_obj, pb_hubs_Hub_reset);

STATIC mp_obj_t pb_hubs_Hub_reset_reason(mp_obj_t self_in) {
    pbdrv_reset_reason_t reason = pbdrv_reset_get_reason();
    return MP_OBJ_NEW_SMALL_INT(reason);
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_hubs_Hub_reset_reason_obj, pb_hubs_Hub_reset_reason);

#endif // PBDRV_CONFIG_RESET

STATIC const mp_rom_map_elem_t hubs_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),              MP_ROM_QSTR(MP_QSTR_hubs)      },
    { MP_ROM_QSTR(PYBRICKS_HUB_CLASS_NAME),       MP_ROM_PTR(&pb_type_SystemHub) },
    #ifdef PYBRICKS_HUB_CLASS_NAME_ALIAS
    { MP_ROM_QSTR(PYBRICKS_HUB_CLASS_NAME_ALIAS), MP_ROM_PTR(&pb_type_SystemHub) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_hubs_globals, hubs_globals_table);

const mp_obj_module_t pb_module_hubs = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_hubs_globals,
};

#endif // PYBRICKS_PY_HUBS
