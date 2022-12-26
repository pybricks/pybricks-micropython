// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_BLE

#include "uzlib.h"

#include "py/objstr.h"

#include <pybricks/ble.h>

#include <pybricks/util_pb/pb_error.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_conversions.h>

// Signal object
typedef struct _broadcast_signal_t {
    uint32_t hash;
    qstr name;
    uint8_t counter;
    uint8_t size;
    uint8_t message[23];
} broadcast_signal_t;

// Class structure for Broadcast
typedef struct _ble_Broadcast_obj_t {
    mp_obj_base_t base;
    size_t n_signals;
    broadcast_signal_t *signals;
} ble_Broadcast_obj_t;

// There is at most one Broadcast object
ble_Broadcast_obj_t *broadcast_obj;

// TODO: Process scan results and populate any broadcast_signal_t whose
// crc32 hash matches the scanned broadcast. This process is started from
// function below.

// pybricks.ble.Broadcast.__init__
STATIC mp_obj_t ble_Broadcast_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(signals));

    // Create the object.
    broadcast_obj = m_new_obj(ble_Broadcast_obj_t);
    broadcast_obj->base.type = (mp_obj_type_t *)type;

    // Unpack signal list.
    mp_obj_t *signal_args;
    mp_obj_get_array(signals_in, &broadcast_obj->n_signals, &signal_args);

    // Allocate space for signals.
    broadcast_obj->signals = m_new(broadcast_signal_t, broadcast_obj->n_signals);

    // Initialize objects.
    for (size_t i = 0; i < broadcast_obj->n_signals; i++) {

        broadcast_signal_t *signal = &broadcast_obj->signals[i];

        // Get signal name info
        signal->name = mp_obj_str_get_qstr(signal_args[i]);
        GET_STR_DATA_LEN(signal_args[i], signal_name, signal_name_len);
        signal->hash = (0xFFFFFFFF & -uzlib_crc32(signal_name, signal_name_len, 0xFFFFFFFF)) - 1;

        // Reset message info.
        signal->counter = 0;
        signal->size = 0;

    }

    // TODO: Call function to start scanning.

    return MP_OBJ_FROM_PTR(broadcast_obj);
}

STATIC broadcast_signal_t *get_signal_by_name(mp_obj_t signal_name_obj) {

    qstr signal_name = mp_obj_str_get_qstr(signal_name_obj);

    // Return signal if known.
    for (size_t i = 0; i < broadcast_obj->n_signals; i++) {
        if (broadcast_obj->signals[i].name == signal_name) {
            return &broadcast_obj->signals[i];
        }
    }
    // Signal not found.
    pb_assert(PBIO_ERROR_INVALID_ARG);
    return NULL;
}

// pybricks.ble.Broadcast.received
STATIC mp_obj_t ble_Broadcast_received(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(signal));

    (void)self;
    broadcast_signal_t *signal = get_signal_by_name(signal_in);
    return mp_obj_new_bytes(signal->message, signal->size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_received_obj, 1, ble_Broadcast_received);

// pybricks.ble.Broadcast.transmit
STATIC mp_obj_t ble_Broadcast_transmit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(signal),
        PB_ARG_REQUIRED(message));

    (void)self;
    broadcast_signal_t *signal = get_signal_by_name(signal_in);

    // Assert that data argument are bytes.
    if (!(mp_obj_is_type(message_in, &mp_type_bytes) || mp_obj_is_type(message_in, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Unpack user argument to update signal.
    mp_obj_str_t *byte_data = MP_OBJ_TO_PTR(message_in);
    if (byte_data->len > 23) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    memcpy(signal->message, byte_data->data, byte_data->len);
    signal->size = byte_data->len;
    signal->counter++;

    (void)self;
    // TODO: Broadcast the updated signal.

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_transmit_obj, 1, ble_Broadcast_transmit);

// dir(pybricks.ble.Broadcast)
STATIC const mp_rom_map_elem_t ble_Broadcast_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_received),       MP_ROM_PTR(&ble_Broadcast_received_obj) },
    { MP_ROM_QSTR(MP_QSTR_transmit),       MP_ROM_PTR(&ble_Broadcast_transmit_obj) },
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
