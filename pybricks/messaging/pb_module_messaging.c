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

#include <pbdrv/bluetooth.h>

#include <pbio/int_math.h>
#include <pbio/util.h>

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

typedef struct {
    mp_obj_base_t base;
    uint32_t num_results;
    uint32_t num_results_max;
    pbdrv_bluetooth_inquiry_result_t results[];
} pb_messaging_bluetooth_scan_result_obj_t;

static mp_obj_t pb_messaging_bluetooth_scan_close(mp_obj_t self_in) {
    pb_messaging_bluetooth_scan_result_obj_t *self = MP_OBJ_TO_PTR(self_in);
    DEBUG_PRINT("rfcomm scan data freed\n");
    self->num_results_max = 0;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_messaging_bluetooth_scan_close_obj, pb_messaging_bluetooth_scan_close);

static const mp_rom_map_elem_t pb_messaging_bluetooth_scan_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_messaging_bluetooth_scan_close_obj) },
};
static MP_DEFINE_CONST_DICT(pb_messaging_bluetooth_scan_locals_dict, pb_messaging_bluetooth_scan_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_messaging_bluetooth_scan, MP_QSTR_bluetooth_scan, MP_TYPE_FLAG_NONE, locals_dict, &pb_messaging_bluetooth_scan_locals_dict);

/**
 * Utility for converting a bluetooth address byte buffer to a string.
 *
 * The result does not persist. Intended for instant consumption.
 *
 * @param  [in]  address 6-byte bluetooth address.
 * @return               Formatted bluetooth address string.
 */
static char *format_bluetooth_address(uint8_t *address) {
    static char bdaddr_str[18];
    snprintf(bdaddr_str, sizeof(bdaddr_str), "%02X:%02X:%02X:%02X:%02X:%02X",
        address[0], address[1], address[2], address[3], address[4], address[5]);
    return bdaddr_str;
}

/**
 * Maps the inquiry results to a list of dictionary, to be returned to the user.
 */
static mp_obj_t pb_messaging_bluetooth_scan_return_map(mp_obj_t parent_obj) {

    pb_messaging_bluetooth_scan_result_obj_t *scanner = MP_OBJ_TO_PTR(parent_obj);

    mp_obj_t list = mp_obj_new_list(0, NULL);

    for (uint32_t i = 0; i < scanner->num_results; i++) {
        mp_obj_t dict = mp_obj_new_dict(0);
        pbdrv_bluetooth_inquiry_result_t *result = &scanner->results[i];
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_address), mp_obj_new_str(format_bluetooth_address(result->bdaddr), 17));
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_name), mp_obj_new_str(result->name, strlen(result->name)));
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_rssi), mp_obj_new_int(result->rssi));
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_class), mp_obj_new_int(result->class_of_device));
        mp_obj_list_append(list, dict);
    }
    return list;
}

// pybricks.messaging.bluetooth_scan
static mp_obj_t pb_messaging_bluetooth_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_INT(timeout, 10000),
        PB_ARG_DEFAULT_INT(num_results, 5));

    // Allocate the maximum number of expected results.
    uint32_t num_results_max = mp_obj_get_int(num_results_in);
    if (!num_results_max) {
        num_results_max = 1;
    }
    pb_messaging_bluetooth_scan_result_obj_t *scanner = mp_obj_malloc_var_with_finaliser(
        pb_messaging_bluetooth_scan_result_obj_t, pbdrv_bluetooth_inquiry_result_t,
        num_results_max, &pb_type_messaging_bluetooth_scan);

    // Initialize at zero results.
    scanner->num_results = 0;
    scanner->num_results_max = mp_obj_get_int(num_results_in);
    pb_assert(pbdrv_bluetooth_start_inquiry_scan(scanner->results, &scanner->num_results, &scanner->num_results_max, mp_obj_get_int(timeout_in)));

    // Create an awaitable with a reference to our result to keep it from being
    // garbage collected.
    pb_type_async_t *iter = NULL;
    pb_type_async_t config = {
        .iter_once = pbdrv_bluetooth_await_classic_task,
        .parent_obj = MP_OBJ_FROM_PTR(scanner),
        .return_map = pb_messaging_bluetooth_scan_return_map,
    };
    return pb_type_async_wait_or_await(&config, &iter, false);
}
// See also messaging_globals_table below. This function object is added there to make it importable.
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_messaging_bluetooth_scan_obj, 0, pb_messaging_bluetooth_scan);

static const mp_rom_map_elem_t messaging_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_messaging) },
    { MP_ROM_QSTR(MP_QSTR_bluetooth_scan), MP_ROM_PTR(&pb_messaging_bluetooth_scan_obj) },
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
