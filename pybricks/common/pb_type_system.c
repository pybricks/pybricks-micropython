// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_SYSTEM

#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbsys/storage.h>

#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

static mp_obj_t pb_type_System_name(void) {
    const char *hub_name = pbdrv_bluetooth_get_hub_name();
    return mp_obj_new_str(hub_name, strlen(hub_name));
}
static MP_DEFINE_CONST_FUN_OBJ_0(pb_type_System_name_obj, pb_type_System_name);

#if PBDRV_CONFIG_RESET

#include <pbdrv/reset.h>

static mp_obj_t pb_type_System_reset_reason(void) {
    pbdrv_reset_reason_t reason = pbdrv_reset_get_reason();
    return MP_OBJ_NEW_SMALL_INT(reason);
}
static MP_DEFINE_CONST_FUN_OBJ_0(pb_type_System_reset_reason_obj, pb_type_System_reset_reason);

#endif // PBDRV_CONFIG_RESET

#if PBIO_CONFIG_ENABLE_SYS

#include <pbsys/status.h>
#include <pbsys/program_stop.h>

#include <pybricks/parameters.h>

static mp_obj_t pb_type_System_set_stop_button(mp_obj_t buttons_in) {
    pbio_button_flags_t buttons = 0;

    if (mp_obj_is_true(buttons_in)) {
        #if PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS > 1
        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            // try an iterator first in case there are multiple buttons
            mp_obj_t iter = mp_getiter(buttons_in, NULL);
            mp_obj_t item;
            while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
                buttons |= pb_type_button_get_button_flag(item);
            }
            nlr_pop();
        } else {
            // mp_getiter() will raise an exception if it is not an iter, so we
            // will end up here in that case, where we are expecting a single
            // button enum value. Technically there could be other error that
            // get us here, but they should be rare and will likely be a ValueError
            // which is the same error that will be raised here.
            buttons = pb_type_button_get_button_flag(buttons_in);
        }
        #else // PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS > 1
        buttons = PBIO_BUTTON_CENTER;
        #endif // PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS > 1
    }

    pbsys_program_stop_set_buttons(buttons);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_System_set_stop_button_obj, pb_type_System_set_stop_button);

static mp_obj_t pb_type_System_shutdown(void) {

    // Start shutdown.
    pbsys_status_set(PBIO_PYBRICKS_STATUS_SHUTDOWN_REQUEST);

    // Keep running MicroPython until we are stopped.
    for (;;) {
        MICROPY_EVENT_POLL_HOOK;
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(pb_type_System_shutdown_obj, pb_type_System_shutdown);

static mp_obj_t pb_type_System_storage(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(offset),
        PB_ARG_DEFAULT_NONE(read),
        PB_ARG_DEFAULT_NONE(write));

    // Get offset and confirm integer type.
    mp_int_t offset = mp_obj_get_int(offset_in);

    // Handle read.
    if (read_in != mp_const_none && write_in == mp_const_none) {
        byte *data;
        mp_uint_t size = mp_obj_get_int(read_in);
        pb_assert(pbsys_storage_get_user_data(offset, &data, size));
        return mp_obj_new_bytes(data, size);
    }

    // Handle write.
    if (write_in != mp_const_none && read_in == mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(write_in, &bufinfo, MP_BUFFER_READ);

        pb_assert(pbsys_storage_set_user_data(offset, bufinfo.buf, bufinfo.len));

        return mp_const_none;
    }

    mp_raise_TypeError(MP_ERROR_TEXT("Must set either read (int) or write (bytes)."));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_System_storage_obj, 0, pb_type_System_storage);

#endif // PBIO_CONFIG_ENABLE_SYS

// dir(pybricks.common.System)
static const mp_rom_map_elem_t common_System_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_type_System_name_obj) },
    #if PBDRV_CONFIG_RESET
    { MP_ROM_QSTR(MP_QSTR_reset_reason), MP_ROM_PTR(&pb_type_System_reset_reason_obj) },
    #endif // PBDRV_CONFIG_RESET
    #if PBIO_CONFIG_ENABLE_SYS
    { MP_ROM_QSTR(MP_QSTR_set_stop_button), MP_ROM_PTR(&pb_type_System_set_stop_button_obj) },
    { MP_ROM_QSTR(MP_QSTR_shutdown), MP_ROM_PTR(&pb_type_System_shutdown_obj) },
    { MP_ROM_QSTR(MP_QSTR_storage), MP_ROM_PTR(&pb_type_System_storage_obj) },
    #endif
};
static MP_DEFINE_CONST_DICT(common_System_locals_dict, common_System_locals_dict_table);

// type(pybricks.common.System) but implemented as module for reduced build size.
// REVISIT: Make implementation consistent across modules/singletons: https://github.com/pybricks/support/issues/840
const mp_obj_module_t pb_type_System = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&common_System_locals_dict,
};

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_SYSTEM
