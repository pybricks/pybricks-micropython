// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_SYSTEM

#include <string.h>

#include <pbdrv/bluetooth.h>

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/util_pb/pb_error.h>

STATIC mp_obj_t pb_type_System_name(void) {
    const char *hub_name = pbdrv_bluetooth_get_hub_name();
    return mp_obj_new_str(hub_name, strlen(hub_name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pb_type_System_name_obj, pb_type_System_name);

#if PBDRV_CONFIG_RESET

#include <pbdrv/reset.h>

STATIC mp_obj_t pb_type_System_reset(mp_obj_t action_in) {
    pbdrv_reset_action_t action = mp_obj_get_int(action_in);

    if (action != PBDRV_RESET_ACTION_RESET_IN_UPDATE_MODE) {
        mp_raise_ValueError(NULL);
    }

    pbdrv_reset(action);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_System_reset_obj, pb_type_System_reset);

STATIC mp_obj_t pb_type_System_reset_reason(void) {
    pbdrv_reset_reason_t reason = pbdrv_reset_get_reason();
    return MP_OBJ_NEW_SMALL_INT(reason);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pb_type_System_reset_reason_obj, pb_type_System_reset_reason);

#endif // PBDRV_CONFIG_RESET

#if PBIO_CONFIG_ENABLE_SYS

#include <pbsys/status.h>
#include <pbsys/program_stop.h>

#include <pybricks/parameters.h>

STATIC mp_obj_t pb_type_System_set_stop_button(mp_obj_t buttons_in) {
    pbio_button_flags_t buttons = 0;

    if (mp_obj_is_true(buttons_in)) {
        #if PYBRICKS_PY_PARAMETERS_BUTTON
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            // try an iterator first in case there are multiple buttons
            mp_obj_t iter = mp_getiter(buttons_in, NULL);
            mp_obj_t item;
            while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
                buttons |= pb_type_enum_get_value(item, &pb_enum_type_Button);
            }
            nlr_pop();
        } else {
            // mp_getiter() will raise an exception if it is not an iter, so we
            // will end up here in that case, where we are expecting a single
            // button enum value. Technically there could be other error that
            // get us here, but they should be rare and will likely be a ValueError
            // which is the same error that will be raised here.
            buttons = pb_type_enum_get_value(buttons_in, &pb_enum_type_Button);
        }
        #else // PYBRICKS_PY_PARAMETERS_BUTTON
        // if the buttons enum is not enabled, then there is only one button
        buttons = PBIO_BUTTON_CENTER;
        #endif // PYBRICKS_PY_PARAMETERS_BUTTON
    }

    pbsys_program_stop_set_buttons(buttons);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_System_set_stop_button_obj, pb_type_System_set_stop_button);

STATIC mp_obj_t pb_type_System_shutdown(void) {

    // Start shutdown.
    pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);

    // Keep running MicroPython until we are stopped.
    for (;;) {
        MICROPY_EVENT_POLL_HOOK;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pb_type_System_shutdown_obj, pb_type_System_shutdown);

#endif // PBIO_CONFIG_ENABLE_SYS

// dir(pybricks.common.System)
STATIC const mp_rom_map_elem_t common_System_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_type_System_name_obj) },
    #if PBDRV_CONFIG_RESET
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&pb_type_System_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_reason), MP_ROM_PTR(&pb_type_System_reset_reason_obj) },
    #endif // PBDRV_CONFIG_RESET
    #if PBIO_CONFIG_ENABLE_SYS
    { MP_ROM_QSTR(MP_QSTR_set_stop_button), MP_ROM_PTR(&pb_type_System_set_stop_button_obj) },
    { MP_ROM_QSTR(MP_QSTR_shutdown), MP_ROM_PTR(&pb_type_System_shutdown_obj) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(common_System_locals_dict, common_System_locals_dict_table);

// type(pybricks.common.System)
const mp_obj_type_t pb_type_System = {
    { &mp_type_type },
    .name = MP_QSTR_System,
    .locals_dict = (mp_obj_dict_t *)&common_System_locals_dict,
};

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_SYSTEM
