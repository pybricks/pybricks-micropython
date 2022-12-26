// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_BLE

#include "uzlib.h"

#include "py/objstr.h"

#include <pbdrv/bluetooth.h>
#include <pbio/broadcast.h>
#include <pbio/util.h>

#include <pybricks/ble.h>

#include <pybricks/util_pb/pb_error.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_conversions.h>
#include <pybricks/util_pb/pb_task.h>

enum {
    /** Zero-terminated string. */
    BROADCAST_DATA_TYPE_STRING = 0x00,

    /** little-endian 16-bit signed integer. */
    BROADCAST_DATA_TYPE_INT16 = 0x01,

    /** little-endian 32-bit signed integer. */
    BROADCAST_DATA_TYPE_INT32 = 0x02,

    /** little-endian 32-bit floating point. */
    BROADCAST_DATA_TYPE_FLOAT = 0x03,
};

#define BROADCAST_MAX_OBJECTS (8)

// Broadcast data format: HEADER + ENCODING + DATA
//
// HEADER (8 bit)
//    BIT 0--3 (LSB): Tuple size.
//    BIT 4--8 (MSB): Reserved.
//
// ENCODING (16 bit little-endian)
//    BIT 0--1   (LSB): Data type of first object in tuple, or type of the single object
//    BIT 2--3        : Data type of second object in tuple
//      ...
//    BIT 15--16 (MSB): Data type of eighth object in tuple
//
// DATA (up to 20 bytes, representing tuple with up to 8 values.)

// Encodes objects into a format for broadcasting
STATIC void broadcast_encode_data(mp_obj_t data_in, uint8_t *dest, uint8_t *len) {

    // Send empty message for None.
    if (data_in == mp_const_none) {
        *len = 0;
        return;
    }

    // If it's a single object, remember this for transmission but treat as tuple anyway.
    bool is_single_object = false;
    if (mp_obj_is_int(data_in) || mp_obj_is_float(data_in) || mp_obj_is_str(data_in)) {
        data_in = mp_obj_new_tuple(1, &data_in);
        is_single_object = true;
    }

    // Iterable buffer.
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t data_iter = mp_getiter(data_in, &iter_buf);

    // Length is at least the header with the encoding
    *len = 3;

    // Reset encoding.
    uint16_t *encoding = (uint16_t *)&dest[1];
    *encoding = 0;

    // Go through all values.
    uint8_t n;
    for (n = 0; n < BROADCAST_MAX_OBJECTS; n++) {
        mp_obj_t obj = mp_iternext(data_iter);

        // Break when iterator complete.
        if (obj == MP_OBJ_STOP_ITERATION) {
            break;
        }

        // Encode integer
        if (mp_obj_is_int(obj)) {
            mp_int_t value = mp_obj_get_int(obj);

            // Check integer size.
            if (value < INT16_MIN || value > INT16_MAX) {
                // Result must be inside max data length.
                if (*len + sizeof(int32_t) > PBIO_BROADCAST_MAX_PAYLOAD_SIZE) {
                    pb_assert(PBIO_ERROR_INVALID_ARG);
                }
                // Encode 32 bit integer.
                *encoding |= BROADCAST_DATA_TYPE_INT32 << n * 2;
                *(int32_t *)&dest[*len] = value;
                *len += sizeof(int32_t);
            } else {
                // Result must be inside max data length.
                if (*len + sizeof(int16_t) > PBIO_BROADCAST_MAX_PAYLOAD_SIZE) {
                    pb_assert(PBIO_ERROR_INVALID_ARG);
                }
                // Encode 16 bit integer.
                *encoding |= BROADCAST_DATA_TYPE_INT16 << n * 2;
                *(int16_t *)&dest[*len] = value;
                *len += sizeof(int16_t);
            }
        } else if (mp_obj_is_float(obj)) {
            // Result must be inside max data length.
            if (*len + sizeof(mp_float_t) > PBIO_BROADCAST_MAX_PAYLOAD_SIZE) {
                pb_assert(PBIO_ERROR_INVALID_ARG);
            }
            // Encode float
            *encoding |= BROADCAST_DATA_TYPE_FLOAT << n * 2;
            // Revisit: Make proper pack/unpack functions.
            mp_float_t f = mp_obj_get_float(obj);
            int32_t *encoded = (int32_t *)&f;
            *(int32_t *)&dest[*len] = *encoded;
            *len += sizeof(mp_float_t);
        } else {
            // get string info.
            GET_STR_DATA_LEN(obj, str_data, str_len);

            // Result must be inside max data length.
            if (*len + str_len + 1 > PBIO_BROADCAST_MAX_PAYLOAD_SIZE) {
                pb_assert(PBIO_ERROR_INVALID_ARG);
            }

            // Copy string including null terminator.
            *encoding |= BROADCAST_DATA_TYPE_STRING << n * 2;
            memcpy(&dest[*len], str_data, str_len + 1);
            *len += str_len + 1;
        }
    }

    // Set header to number of objects, or 0 if it is not a tuple.
    dest[0] = is_single_object ? 0 : n;
}

STATIC mp_obj_t broadcast_decode_data(uint8_t *data, uint8_t len) {

    // Pybricks data requires at least 4 bytes
    if (len < 4) {
        return mp_const_none;
    }

    // Get encoding data
    uint16_t encoding = *(uint16_t *)&data[1];

    // If tuple size is 0 but there is nonzero data, it's a single value.
    // We'll remember this for the return value but but treat it as a tuple.
    // while unpacking the data.
    bool is_single_object = data[0] == 0 && len > 3;
    uint8_t n = is_single_object ? 1 : data[0];

    // Check that objects are within bounds.
    if (n > BROADCAST_MAX_OBJECTS) {
        return mp_const_none;
    }

    // Populate all objects
    uint8_t *value = &data[3];
    mp_obj_t objs[BROADCAST_MAX_OBJECTS];
    for (uint8_t i = 0; i < n; i++) {
        // Decode object based on data type.
        switch ((encoding >> i * 2) & 0x03) {
            case BROADCAST_DATA_TYPE_STRING: {
                uint8_t str_size = strlen((char *)value);
                objs[i] = mp_obj_new_str((char *)value, str_size);
                value += str_size + 1;
                break;
            }
            case BROADCAST_DATA_TYPE_INT16:
                objs[i] = mp_obj_new_int(*(int16_t *)value);
                value += sizeof(int16_t);
                break;
            case BROADCAST_DATA_TYPE_INT32:
                objs[i] = mp_obj_new_int(*(int32_t *)value);
                value += sizeof(int32_t);
                break;
            case BROADCAST_DATA_TYPE_FLOAT: {
                // Revisit: Make proper pack/unpack functions.
                uint32_t encoded = *(uint32_t *)value;
                mp_float_t *f = (mp_float_t *)&encoded;
                objs[i] = mp_obj_new_float(*f);
                value += sizeof(mp_float_t);
                break;
            }
        }
    }

    // Return the single object or tuple
    return is_single_object ? objs[0] : mp_obj_new_tuple(n, objs);
}

// Class structure for Broadcast
typedef struct _ble_Broadcast_obj_t {
    mp_obj_base_t base;
    size_t num_topics;
    qstr *topics;
    uint32_t *hashes;
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
        PB_ARG_REQUIRED(topics));

    // Clear old broadcast data.
    pbio_broadcast_clear_all();

    // Create the object.
    broadcast_obj = m_new_obj(ble_Broadcast_obj_t);
    broadcast_obj->base.type = (mp_obj_type_t *)type;

    // Unpack signal list.
    mp_obj_t *signal_args;
    mp_obj_get_array(topics_in, &broadcast_obj->num_topics, &signal_args);

    // Allocate space for signal names. We can't use a simple dictionary
    // because long ints are not enabled.
    broadcast_obj->topics = m_new(qstr, broadcast_obj->num_topics);
    broadcast_obj->hashes = m_new(uint32_t, broadcast_obj->num_topics);

    // Initialize objects.
    for (size_t i = 0; i < broadcast_obj->num_topics; i++) {

        // Store signal name as qstring
        broadcast_obj->topics[i] = mp_obj_str_get_qstr(signal_args[i]);

        // Get signal name info
        GET_STR_DATA_LEN(signal_args[i], signal_name, signal_name_len);
        broadcast_obj->hashes[i] = (0xFFFFFFFF & -uzlib_crc32(signal_name, signal_name_len, 0xFFFFFFFF)) - 1;

        pb_assert(pbio_broadcast_register_signal(broadcast_obj->hashes[i]));
    }

    // Start scanning.
    start_scan(true);

    return MP_OBJ_FROM_PTR(broadcast_obj);
}

STATIC uint32_t broadcast_get_hash(mp_obj_t topic_in) {

    qstr topic = mp_obj_str_get_qstr(topic_in);

    // Return signal if known.
    for (size_t i = 0; i < broadcast_obj->num_topics; i++) {
        if (broadcast_obj->topics[i] == topic) {
            return broadcast_obj->hashes[i];
        }
    }
    // Signal not found.
    mp_raise_ValueError(MP_ERROR_TEXT("Unknown topic."));
}

// pybricks.ble.Broadcast.receive_bytes
STATIC mp_obj_t ble_Broadcast_receive_bytes(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(topic));

    (void)self;

    uint8_t *data;
    uint8_t size;
    pbio_broadcast_receive(broadcast_get_hash(topic_in), &data, &size);

    // Return bytes as-is.
    return mp_obj_new_bytes(data, size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_receive_bytes_obj, 1, ble_Broadcast_receive_bytes);

// pybricks.ble.Broadcast.receive
STATIC mp_obj_t ble_Broadcast_receive(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(topic));

    (void)self;

    uint8_t *data;
    uint8_t size;
    pbio_broadcast_receive(broadcast_get_hash(topic_in), &data, &size);

    // Return decoded data
    return broadcast_decode_data(data, size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_receive_obj, 1, ble_Broadcast_receive);

// pybricks.ble.Broadcast.send_bytes
STATIC mp_obj_t ble_Broadcast_send_bytes(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(topic),
        PB_ARG_REQUIRED(data));

    (void)self;
    // Assert that data argument are bytes.
    if (!(mp_obj_is_type(data_in, &mp_type_bytes) || mp_obj_is_type(data_in, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Transmit bytes as-is.
    mp_obj_str_t *byte_data = MP_OBJ_TO_PTR(data_in);
    pbio_broadcast_transmit(broadcast_get_hash(topic_in), byte_data->data, byte_data->len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_send_bytes_obj, 1, ble_Broadcast_send_bytes);

// pybricks.ble.Broadcast.send
STATIC mp_obj_t ble_Broadcast_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(topic),
        PB_ARG_REQUIRED(data));

    (void)self;

    // Encode the data
    uint8_t buf[PBIO_BROADCAST_MAX_PAYLOAD_SIZE];
    uint8_t size;
    broadcast_encode_data(data_in, buf, &size);

    // Transmit it.
    pbio_broadcast_transmit(broadcast_get_hash(topic_in), buf, size);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_send_obj, 1, ble_Broadcast_send);

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
    { MP_ROM_QSTR(MP_QSTR_receive),        MP_ROM_PTR(&ble_Broadcast_receive_obj)       },
    { MP_ROM_QSTR(MP_QSTR_receive_bytes),  MP_ROM_PTR(&ble_Broadcast_receive_bytes_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),           MP_ROM_PTR(&ble_Broadcast_send_obj)          },
    { MP_ROM_QSTR(MP_QSTR_send_bytes),     MP_ROM_PTR(&ble_Broadcast_send_bytes_obj)    },
    { MP_ROM_QSTR(MP_QSTR_scan),           MP_ROM_PTR(&ble_Broadcast_scan_obj)          },
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
