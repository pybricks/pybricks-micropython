// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_MESSAGING

#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include <pbio/bluetooth.h>

#include <pbio/int_math.h>
#include <pbio/util.h>

#include <pybricks/messaging/messaging.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/tools/pb_type_async.h>

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

static const mp_rom_map_elem_t messaging_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_messaging) },
    #if PYBRICKS_PY_MESSAGING_APP_DATA
    { MP_ROM_QSTR(MP_QSTR_AppData),  MP_ROM_PTR(&pb_type_app_data) },
    #endif // PYBRICKS_PY_MESSAGING_APP_DATA
    #if PYBRICKS_PY_MESSAGING_BLE_RADIO
    { MP_ROM_QSTR(MP_QSTR_BLERadio), MP_ROM_PTR(&pb_type_ble_radio) },
    #endif // PYBRICKS_PY_MESSAGING_BLE_RADIO
};
static MP_DEFINE_CONST_DICT(pb_module_messaging_globals, messaging_globals_table);

const mp_obj_module_t pb_module_messaging = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_messaging_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_messaging, pb_module_messaging);
#endif

#endif // PYBRICKS_PY_MESSAGING
