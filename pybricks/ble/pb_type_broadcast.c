// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_BLE

#include "uzlib.h"

#include "py/objstr.h"

#include <pbdrv/bluetooth.h>
#include <pbio/broadcast.h>

#include <pybricks/ble.h>

#include <pybricks/util_pb/pb_error.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_conversions.h>
#include <pybricks/util_pb/pb_task.h>

// Class structure for Broadcast
typedef struct _ble_Broadcast_obj_t {
    mp_obj_base_t base;
    size_t num_signals;
    qstr *signal_names;
    uint32_t *signal_hashes;
} ble_Broadcast_obj_t;

// There is at most one Broadcast object
ble_Broadcast_obj_t *broadcast_obj;

STATIC void start_scan(bool start) {
    // Start scanning and wait for completion.
    pbio_task_t task;
    pbdrv_bluetooth_start_scan(&task, start);
    pb_wait_task(&task, -1);
}

// pybricks.ble.Broadcast.__init__
STATIC mp_obj_t ble_Broadcast_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(signals));

    // Clear old broadcast data.
    pbio_broadcast_clear_all();

    // Create the object.
    broadcast_obj = m_new_obj(ble_Broadcast_obj_t);
    broadcast_obj->base.type = (mp_obj_type_t *)type;

    // Unpack signal list.
    mp_obj_t *signal_args;
    mp_obj_get_array(signals_in, &broadcast_obj->num_signals, &signal_args);

    // Allocate space for signal names. We can't use a simple dictionary
    // because long ints are not enabled.
    broadcast_obj->signal_names = m_new(qstr, broadcast_obj->num_signals);
    broadcast_obj->signal_hashes = m_new(uint32_t, broadcast_obj->num_signals);

    // Initialize objects.
    for (size_t i = 0; i < broadcast_obj->num_signals; i++) {

        // Store signal name as qstring
        broadcast_obj->signal_names[i] = mp_obj_str_get_qstr(signal_args[i]);

        // Get signal name info
        GET_STR_DATA_LEN(signal_args[i], signal_name, signal_name_len);
        broadcast_obj->signal_hashes[i] = (0xFFFFFFFF & -uzlib_crc32(signal_name, signal_name_len, 0xFFFFFFFF)) - 1;

        pb_assert(pbio_broadcast_register_signal(broadcast_obj->signal_hashes[i]));
    }

    // Start scanning.
    start_scan(true);

    return MP_OBJ_FROM_PTR(broadcast_obj);
}

STATIC uint32_t get_hash_by_name(mp_obj_t signal_name_obj) {

    qstr signal_name = mp_obj_str_get_qstr(signal_name_obj);

    // Return signal if known.
    for (size_t i = 0; i < broadcast_obj->num_signals; i++) {
        if (broadcast_obj->signal_names[i] == signal_name) {
            return broadcast_obj->signal_hashes[i];
        }
    }
    // Signal not found.
    pb_assert(PBIO_ERROR_INVALID_ARG);
    return 0;
}

// pybricks.ble.Broadcast.received
STATIC mp_obj_t ble_Broadcast_received(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(signal));

    (void)self;
    pbio_broadcast_received_t *signal;
    pb_assert(pbio_broadcast_get_signal(&signal, get_hash_by_name(signal_in)));
    return mp_obj_new_bytes(signal->payload, signal->size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_received_obj, 1, ble_Broadcast_received);

// pybricks.ble.Broadcast.transmit
STATIC mp_obj_t ble_Broadcast_transmit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(signal),
        PB_ARG_REQUIRED(message));

    (void)self;
    // Assert that data argument are bytes.
    if (!(mp_obj_is_type(message_in, &mp_type_bytes) || mp_obj_is_type(message_in, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Unpack user argument to update signal.
    mp_obj_str_t *byte_data = MP_OBJ_TO_PTR(message_in);
    pbio_broadcast_transmit(get_hash_by_name(signal_in), byte_data->data, byte_data->len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_transmit_obj, 1, ble_Broadcast_transmit);

// pybricks.ble.Broadcast.scan
STATIC mp_obj_t ble_Broadcast_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(scan));
    (void)self;
    start_scan(mp_obj_is_true(scan_in));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_scan_obj, 1, ble_Broadcast_scan);

// dir(pybricks.ble.Broadcast)
STATIC const mp_rom_map_elem_t ble_Broadcast_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_received),       MP_ROM_PTR(&ble_Broadcast_received_obj) },
    { MP_ROM_QSTR(MP_QSTR_transmit),       MP_ROM_PTR(&ble_Broadcast_transmit_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan),           MP_ROM_PTR(&ble_Broadcast_scan_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ble_Broadcast_locals_dict, ble_Broadcast_locals_dict_table);

// type(pybricks.ble.Broadcast)
const mp_obj_type_t pb_type_Broadcast = {
    { &mp_type_type },
    .name = MP_QSTR_Broadcast,
    .make_new = ble_Broadcast_make_new,
    .locals_dict = (mp_obj_dict_t *)&ble_Broadcast_locals_dict,
};

#endif // PYBRICKS_PY_BLE
