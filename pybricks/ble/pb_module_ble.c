// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_BLE

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/ble.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

STATIC const mp_rom_map_elem_t ble_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),     MP_ROM_QSTR(MP_QSTR_ble)       },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_ble_globals, ble_globals_table);

const mp_obj_module_t pb_module_ble = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_ble_globals,
};

MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_ble, pb_module_ble);

#endif // PYBRICKS_PY_BLE
