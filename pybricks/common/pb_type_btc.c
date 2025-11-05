// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_BTC

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <pbdrv/bluetooth.h>

#include <pbsys/config.h>
#include <pbsys/status.h>

#include "py/obj.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_async.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct {
    mp_obj_base_t base;
    pb_type_async_t *scan_awaitable;
} pb_obj_BTC_t;

/**
 * Handles inquiry result from the bluetooth driver.
 *
 * @param [in]  context  The context object (list to append results to).
 * @param [in]  result   The inquiry result from the Bluetooth Classic scan.
 */
static void handle_inquiry_result(void *context, const pbdrv_bluetooth_inquiry_result_t *result) {
    mp_obj_t parent_obj = (mp_obj_t)context;
    mp_obj_t result_dict = mp_obj_new_dict(0);
    char bdaddr_str[18];
    snprintf(bdaddr_str, sizeof(bdaddr_str), "%02x:%02x:%02x:%02x:%02x:%02x",
        result->bdaddr[5], result->bdaddr[4], result->bdaddr[3],
        result->bdaddr[2], result->bdaddr[1], result->bdaddr[0]);
    mp_obj_dict_store(result_dict,
        MP_ROM_QSTR(MP_QSTR_address),
        mp_obj_new_str(bdaddr_str, strlen(bdaddr_str)));
    mp_obj_dict_store(result_dict,
        MP_ROM_QSTR(MP_QSTR_name),
        mp_obj_new_str(result->name, strlen(result->name)));
    mp_obj_dict_store(result_dict,
        MP_ROM_QSTR(MP_QSTR_rssi),
        mp_obj_new_int(result->rssi));
    mp_obj_dict_store(result_dict,
        MP_ROM_QSTR(MP_QSTR_class_of_device),
        mp_obj_new_int(result->class_of_device));
    mp_obj_list_append(parent_obj, result_dict);
}

/**
 * Protothread function that performs the Bluetooth Classic scan.
 *
 * @param [in]  state      The protothread state.
 * @param [in]  parent_obj The BTC object that initiated the scan.
 * @return                 ::PBIO_ERROR_AGAIN while scanning,
 *                         ::PBIO_SUCCESS when scan is complete.
 */
static pbio_error_t pb_type_btc_scan_iterate(pbio_os_state_t *state, mp_obj_t parent_obj) {
    return pbdrv_bluetooth_inquiry_scan(state, -1, 10, parent_obj, handle_inquiry_result);
}

/**
 * Creates the return value for the scan operation.
 *
 * @param [in]  parent_obj The list that we're to return..
 * @return                 A Python list containing dictionaries with scan results.
 */
static mp_obj_t pb_type_btc_scan_return_map(mp_obj_t parent_obj) {
    return parent_obj;
}

/**
 * Closes/cancels the BTC operation.
 *
 * @param [in]  self_in The BTC MicroPython object instance.
 * @return              mp_const_none.
 */
static mp_obj_t pb_type_btc_close(mp_obj_t self_in) {
    return mp_const_none;
}

/**
 * Starts a Bluetooth Classic inquiry scan.
 *
 * @param [in]  self_in The BTC MicroPython object instance.
 * @return              An awaitable that will return a list of scan results.
 */
static mp_obj_t pb_type_btc_scan(mp_obj_t self_in) {
    pb_obj_BTC_t *self = MP_OBJ_TO_PTR(self_in);

    pb_type_async_t config = {
        .iter_once = pb_type_btc_scan_iterate,
        .parent_obj = mp_obj_new_list(0, NULL),
        .close = pb_type_btc_close,
        .return_map = pb_type_btc_scan_return_map,
    };

    return pb_type_async_wait_or_await(&config, &self->scan_awaitable, true);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_btc_scan_obj, pb_type_btc_scan);

/**
 * Close method for cleanup.
 *
 * @param [in]  self_in The BTC MicroPython object instance.
 * @return              mp_const_none.
 */
static mp_obj_t pb_type_btc_del(mp_obj_t self_in) {
    return pb_type_btc_close(self_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_btc_del_obj, pb_type_btc_del);

/**
 * Gets the Bluetooth firmware version.
 *
 * @param [in]  self_in The BTC MicroPython object instance.
 * @return              MicroPython object containing the firmware version as a str.
 */
static mp_obj_t pb_type_btc_version(mp_obj_t self_in) {
    const char *version = pbdrv_bluetooth_get_fw_version();
    return mp_obj_new_str(version, strlen(version));
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_btc_version_obj, pb_type_btc_version);

static const mp_rom_map_elem_t common_BTC_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&pb_type_btc_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&pb_type_btc_version_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_type_btc_del_obj) },
};
static MP_DEFINE_CONST_DICT(common_BTC_locals_dict, common_BTC_locals_dict_table);

static MP_DEFINE_CONST_OBJ_TYPE(pb_type_BTC,
    MP_QSTR_BTC,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_BTC_locals_dict);

/**
 * Creates a new instance of the BTC class.
 *
 * @return  A newly allocated BTC object.
 */
mp_obj_t pb_type_BTC_new(void) {
    pb_obj_BTC_t *self = mp_obj_malloc_with_finaliser(pb_obj_BTC_t, &pb_type_BTC);

    self->scan_awaitable = NULL;

    return MP_OBJ_FROM_PTR(self);
}

#endif // PYBRICKS_PY_COMMON_BTC
