
// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_BLE

#include <string.h>

#include <pbdrv/bluetooth.h>

#include "py/obj.h"
#include "py/misc.h"
#include "py/runtime.h"

STATIC mp_obj_t pb_module_ble_version(void) {
    const char *version = pbdrv_bluetooth_get_fw_version();
    return mp_obj_new_str(version, strlen(version));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(pb_module_ble_version_obj, pb_module_ble_version);

STATIC const mp_rom_map_elem_t common_Ble_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&pb_module_ble_version_obj) },
};
STATIC MP_DEFINE_CONST_DICT(common_Ble_locals_dict, common_Ble_locals_dict_table);

const mp_obj_module_t pb_module_ble = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&common_Ble_locals_dict,
};

#endif // PYBRICKS_PY_COMMON_BLE
