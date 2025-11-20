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
#include <py/mperrno.h>

// Register allocated global variables as GC roots since they don't otherwise
// have any parent object to reference them.
MP_REGISTER_ROOT_POINTER(void *scan_pending_results);
MP_REGISTER_ROOT_POINTER(void *scan_awaitable);

#define GET_SCAN_PENDING_RESULTS() ((pbdrv_bluetooth_inquiry_result_t *)MP_STATE_VM(scan_pending_results))
#define SET_SCAN_PENDING_RESULTS(ptr) (MP_STATE_VM(scan_pending_results) = (struct pbdrv_bluetooth_inquiry_result_t *)(ptr))
#define GET_SCAN_AWAITABLE() ((pb_type_async_t *)MP_STATE_VM(scan_awaitable))
#define SET_SCAN_AWAITABLE(ptr) (MP_STATE_VM(scan_awaitable) = (pb_type_async_t *)(ptr))

static size_t scan_results_max_size = 0;
static size_t scan_results_current_index = 0;
static int32_t scan_timeout = 10;

/**
 * Handles inquiry result from the bluetooth driver.
 *
 * @param [in]  context  The result list (unused in this phase).
 * @param [in]  result   The inquiry result from the Bluetooth Classic scan.
 */
static void handle_inquiry_result(void *context, const pbdrv_bluetooth_inquiry_result_t *result) {
    if (result == NULL) {
        return;
    }

    // If we haven't reached the maximum number of results and allocation is valid, store this one
    if (GET_SCAN_PENDING_RESULTS() != NULL && scan_results_current_index < scan_results_max_size) {
        memcpy(&GET_SCAN_PENDING_RESULTS()[scan_results_current_index], result, sizeof(pbdrv_bluetooth_inquiry_result_t));
        scan_results_current_index++;
    }
}

static pbio_error_t pb_type_btc_scan_iterate(pbio_os_state_t *state, mp_obj_t parent_obj) {
    return pbdrv_bluetooth_inquiry_scan(state, (int32_t)scan_results_max_size, scan_timeout, NULL, handle_inquiry_result);
}

/**
 * Creates the return value for the scan operation.
 *
 * @param [in]  parent_obj The result list that will be populated.
 * @return                 A Python list containing dictionaries with scan results.
 */
static mp_obj_t pb_type_btc_scan_return_map(mp_obj_t result_list) {
    // Convert each stored result to a Python dictionary and add to the list
    if (GET_SCAN_PENDING_RESULTS() != NULL) {
        for (size_t i = 0; i < scan_results_current_index; i++) {
            const pbdrv_bluetooth_inquiry_result_t *result = &GET_SCAN_PENDING_RESULTS()[i];
            mp_obj_t result_dict = mp_obj_new_dict(0);

            // Format Bluetooth address
            char bdaddr_str[18];
            snprintf(bdaddr_str, sizeof(bdaddr_str), "%02x:%02x:%02x:%02x:%02x:%02x",
                result->bdaddr[5], result->bdaddr[4], result->bdaddr[3],
                result->bdaddr[2], result->bdaddr[1], result->bdaddr[0]);

            // Add fields to dictionary
            mp_obj_dict_store(result_dict,
                MP_ROM_QSTR(MP_QSTR_address),
                mp_obj_new_str(bdaddr_str, strlen(bdaddr_str)));
            mp_obj_dict_store(result_dict,
                MP_ROM_QSTR(MP_QSTR_name),
                mp_obj_new_bytes((const byte *)result->name, strlen(result->name)));
            mp_obj_dict_store(result_dict,
                MP_ROM_QSTR(MP_QSTR_rssi),
                mp_obj_new_int(result->rssi));
            mp_obj_dict_store(result_dict,
                MP_ROM_QSTR(MP_QSTR_class_of_device),
                mp_obj_new_int(result->class_of_device));
            mp_obj_list_append(result_list, result_dict);
        }

        // Free the allocated memory
        m_del(pbdrv_bluetooth_inquiry_result_t, GET_SCAN_PENDING_RESULTS(), scan_results_max_size);
        SET_SCAN_PENDING_RESULTS(NULL);
    }

    return result_list;
}

/**
 * Starts a Bluetooth Classic inquiry scan.
 *
 * @param [in]  n_args      Number of positional arguments.
 * @param [in]  pos_args    Positional arguments (none expected).
 * @param [in]  kw_args     Keyword arguments (max_devices and timeout are optional).
 *                          max_devices (default: 20) - Maximum number of devices to find
 *                          timeout (default: 10) - Scan timeout in seconds
 * @return                  An awaitable that will return a list of scan results.
 */
static mp_obj_t pb_type_btc_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Define the allowed keyword arguments
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_max_devices, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 20} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 10} },
    };

    // Parse the keyword arguments
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get max_devices parameter
    int32_t max_devices = args[0].u_int;
    if (max_devices < 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("max_devices must be at least 1"));
        return MP_OBJ_NULL;
    }

    // Get timeout parameter
    scan_timeout = args[1].u_int;
    if (scan_timeout < 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be at least 1"));
        return MP_OBJ_NULL;
    }

    // Free any previously allocated memory
    if (GET_SCAN_PENDING_RESULTS() != NULL) {
        m_del(pbdrv_bluetooth_inquiry_result_t, GET_SCAN_PENDING_RESULTS(), scan_results_max_size);
        SET_SCAN_PENDING_RESULTS(NULL);
    }

    // Allocate memory for the scan results
    SET_SCAN_PENDING_RESULTS(m_new(pbdrv_bluetooth_inquiry_result_t, max_devices));
    if (GET_SCAN_PENDING_RESULTS() == NULL) {
        mp_raise_OSError(MP_ENOMEM);
    }
    // Initialize variables for this scan
    scan_results_max_size = (size_t)max_devices;
    scan_results_current_index = 0;

    // Create the result list that will be populated during scanning
    mp_obj_t result_list = mp_obj_new_list(0, NULL);

    pb_type_async_t config = {
        .iter_once = pb_type_btc_scan_iterate,
        .parent_obj = result_list,  // Pass the result list as parent_obj
        .close = NULL,  // No close function needed for module
        .return_map = pb_type_btc_scan_return_map,
    };

    pb_type_async_t *scan_awaitable_ptr = GET_SCAN_AWAITABLE();
    mp_obj_t result = pb_type_async_wait_or_await(&config, &scan_awaitable_ptr, true);
    SET_SCAN_AWAITABLE(scan_awaitable_ptr);

    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_btc_scan_obj, 0, pb_type_btc_scan);

static const mp_rom_map_elem_t common_BTC_globals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_btc) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&pb_type_btc_scan_obj) },
};
static MP_DEFINE_CONST_DICT(common_BTC_globals_dict, common_BTC_globals_dict_table);

const mp_obj_module_t pb_module_btc = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&common_BTC_globals_dict,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_experimental_dot_btc, pb_module_btc);
#endif

#endif // PYBRICKS_PY_COMMON_BTC
