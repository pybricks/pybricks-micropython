
// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_BLE

#include <assert.h>
#include <string.h>

#include <pbdrv/bluetooth.h>

#include <pbsys/config.h>
#include <pbsys/storage_settings.h>

#include "py/obj.h"
#include "py/misc.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_pb/pb_error.h>

// The code currently passes integers and floats directly as bytes so requires
// little-endian to get the correct ordering over the air.
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "this module requires little endian processor"
#endif

// RSSI is smoothed across a time window to reduce jitter.
#define RSSI_FILTER_WINDOW_MS (512)

#define OBSERVED_DATA_TIMEOUT_MS (1000)
#define OBSERVED_DATA_MAX_SIZE (31 /* max adv data size */ - 5 /* overhead */)

typedef struct {
    uint32_t timestamp;
    uint8_t channel;
    int8_t rssi;
    uint8_t size;
    uint8_t data[OBSERVED_DATA_MAX_SIZE];
} observed_data_t;

// pointer to dynamically allocated memory - needed for driver callback
static observed_data_t *observed_data;
static uint8_t num_observed_data;

static pbio_task_t broadcast_task;

typedef struct {
    mp_obj_base_t base;
    uint8_t broadcast_channel;
    pbio_task_t *broadcast_task;
    observed_data_t observed_data[];
} pb_obj_BLE_t;

/**
 * Type codes used for encoding/decoding data.
 */
typedef enum {
    // NB: These values are sent over the air so the numeric values must not be changed.
    // There can be at most 8 types since the values have to fit in 3 bits.

    /** Indicator that the next value is the one and only value (instead of a tuple). */
    PB_BLE_BROADCAST_DATA_TYPE_SINGLE_OBJECT = 0,
    /** The Python @c True value. */
    PB_BLE_BROADCAST_DATA_TYPE_TRUE = 1,
    /** The Python @c False value. */
    PB_BLE_BROADCAST_DATA_TYPE_FALSE = 2,
    /** The Python @c int type. */
    PB_BLE_BROADCAST_DATA_TYPE_INT = 3,
    /** The Python @c float type. */
    PB_BLE_BROADCAST_DATA_TYPE_FLOAT = 4,
    /** The Python @c str type. */
    PB_BLE_BROADCAST_DATA_TYPE_STR = 5,
    /** The Python @c bytes type. */
    PB_BLE_BROADCAST_DATA_TYPE_BYTES = 6,
} pb_ble_broadcast_data_type_t;

#define MFG_SPECIFIC 0xFF
#define LEGO_CID 0x0397

/**
 * Looks up a channel in the observed data table.
 *
 * @param [in]  channel     The channel number (1 to 255).
 * @returns                 A pointer to the channel or @c NULL if the channel
 *                          is not allocated in the table.
 */
static observed_data_t *lookup_observed_data(uint8_t channel) {
    for (size_t i = 0; i < num_observed_data; i++) {
        observed_data_t *data = &observed_data[i];

        if (data->channel == channel) {
            return data;
        }
    }

    return NULL;
}

/**
 * Handles observe event from the bluetooth driver.
 *
 * The advertising data is parsed and if it matches the required format, it is
 * saved in the observed_data table for later use.
 *
 * @param [in]  event_type      The BLE advertisement event type.
 * @param [in]  data            The raw advertising data.
 * @param [in]  length          The length of @p data in bytes.
 * @param [in]  rssi            The RSSI of the event in dBm.
 */
static void handle_observe_event(pbdrv_bluetooth_ad_type_t event_type, const uint8_t *data, uint8_t length, int8_t rssi) {
    // NB: ideally we would also be checking `event_type == PBDRV_BLUETOOTH_AD_TYPE_ADV_NONCONN_IND`
    // here but due to a Bluetooth firmware bug on city hub, we have to allow other
    // advertisement types. This would filter out broadcasts from the experimental
    // feature in official LEGO Robot Inventor firmware.
    if (length >= 5 && data[1] == MFG_SPECIFIC && pbio_get_uint16_le(&data[2]) == LEGO_CID) {
        uint8_t channel = data[4];

        observed_data_t *ch_data = lookup_observed_data(channel);

        // Ignore not allocated channels.
        if (!ch_data) {
            return;
        }

        // Get time difference between subsequent samples.
        uint32_t diff = mp_hal_ticks_ms() - ch_data->timestamp;
        ch_data->timestamp += diff;
        if (diff > RSSI_FILTER_WINDOW_MS) {
            diff = RSSI_FILTER_WINDOW_MS;
        }

        // Update moving RSSI average based on time difference.
        ch_data->rssi = (ch_data->rssi * (RSSI_FILTER_WINDOW_MS - diff) + rssi * diff) / RSSI_FILTER_WINDOW_MS;

        // Extract user broadcast data from signal.
        ch_data->size = data[0] - 4;
        memcpy(ch_data->data, &data[5], OBSERVED_DATA_MAX_SIZE);
    }
}

/**
 * Appends the value of a Python object to the advertising data.
 *
 * @param [in]  dst     Pointer to the start of the manufacturer-specific advertising data.
 * @param [in]  index   The index in @p dst where the value should be written.
 * @param [in]  src     The value to write.
 * @param [in]  size    The size of @p src in bytes.
 * @param [in]  type    The data type of @p src.
 * @returns             The next free index in @p dst after adding the new data.
 * @throws ValueError   If data exceeds available space remaining in @p dst.
 */
static size_t pb_module_ble_append(uint8_t *dst, size_t index, const void *src, size_t size, pb_ble_broadcast_data_type_t type) {
    size_t next_index = index + size + 1;

    if (next_index > OBSERVED_DATA_MAX_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("payload limited to 26 bytes"));
    }

    dst[index] = type << 5 | size;
    memcpy(&dst[index + 1], src, size);

    return next_index;
}

/**
 * Encodes a Python object using the Pybricks Broadcast encoding scheme and
 * appends it to the advertising data.
 *
 * @p arg must be @c True, @c False, an @c int, a @c float, a @c str
 * or bytes-like (supports buffer protocol).
 *
 * @param [in]  dst     Pointer to the start of the manufacturer-specific advertising data.
 * @param [in]  index   The index in @p dst where the value should be written.
 * @param [in]  arg     The Python object to be encoded.
 * @returns             The next free index in @p dst after adding the new data.
 * @throws ValueError   If data exceeds available space remaining in @p dst.
 * @throws TypeError    If @p arg is not one of the supported types.
 */
static size_t pb_module_ble_encode(void *dst, size_t index, mp_obj_t arg) {

    if (arg == mp_const_true) {
        return pb_module_ble_append(dst, index, NULL, 0, PB_BLE_BROADCAST_DATA_TYPE_TRUE);
    }

    if (arg == mp_const_false) {
        return pb_module_ble_append(dst, index, NULL, 0, PB_BLE_BROADCAST_DATA_TYPE_FALSE);
    }

    mp_int_t int_value;
    if (mp_obj_get_int_maybe(arg, &int_value)) {
        if (int_value >= INT8_MIN && int_value <= INT8_MAX) {
            int8_t int8_value = int_value;
            return pb_module_ble_append(dst, index, &int8_value, sizeof(int8_value), PB_BLE_BROADCAST_DATA_TYPE_INT);
        }

        if (int_value >= INT16_MIN && int_value <= INT16_MAX) {
            int16_t int16_value = int_value;
            return pb_module_ble_append(dst, index, &int16_value, sizeof(int16_value), PB_BLE_BROADCAST_DATA_TYPE_INT);
        }

        #if __SIZEOF_POINTER__ == 4
        return pb_module_ble_append(dst, index, &int_value, sizeof(int_value), PB_BLE_BROADCAST_DATA_TYPE_INT);
        #else
        if (int_value >= INT32_MIN && int_value <= INT32_MAX) {
            int32_t int32_value = int_value;
            return pb_module_ble_append(dst, index, &int32_value, sizeof(int32_value), PB_BLE_BROADCAST_DATA_TYPE_INT);
        }

        mp_raise_msg(&mp_type_OverflowError, MP_ERROR_TEXT("integers are limited to 32 bits"));
        #endif
    }

    #if MICROPY_FLOAT_IMPL != MICROPY_FLOAT_IMPL_NONE
    mp_float_t float_value;
    if (mp_obj_get_float_maybe(arg, &float_value)) {
        #if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
        float single_value = float_value;
        return pb_module_ble_append(dst, index, &single_value, sizeof(single_value), PB_BLE_BROADCAST_DATA_TYPE_FLOAT);
        #elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
        return pb_module_ble_append(dst, index, &float_value, sizeof(float_value), PB_BLE_BROADCAST_DATA_TYPE_FLOAT);
        #else
        #error "unsupported MICROPY_FLOAT_IMPL"
        #endif
    }
    #endif

    mp_buffer_info_t info;
    if (mp_get_buffer(arg, &info, MP_BUFFER_READ)) {
        // REVISIT: possible upstream contribution - add str type to info.typecode
        bool is_str = mp_obj_is_str(arg);
        return pb_module_ble_append(dst, index, info.buf, info.len, is_str ? PB_BLE_BROADCAST_DATA_TYPE_STR : PB_BLE_BROADCAST_DATA_TYPE_BYTES);
    }

    mp_raise_TypeError(MP_ERROR_TEXT("must be True, False, int, float, str or bytes"));

    MP_UNREACHABLE
}

/**
 * Sets the broadcast advertising data and enables broadcasting on the Bluetooth
 * radio if it is not already enabled.
 *
 * The data can be one object of the allowed types, or a tuple/list thereof.
 *
 * @param [in]  n_args   The number of args.
 * @param [in]  pos_args The args passed in Python code.
 * @param [in]  kw_args  The kwargs passed in Python code.
 * @throws ValueError    If the channel is out of range or the encoded arguments
 *                       exceed the available space.
 * @throws TypeError     If any of the arguments are of a type that can't be encoded.
 */
static mp_obj_t pb_module_ble_broadcast(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_obj_BLE_t, self,
        PB_ARG_REQUIRED(data));
    // On Move Hub, nothing is broadcast if it is called while the
    // move hub is connected to Pybricks Code. Also, broadcasting interferes
    // with observing even when not connected to Pybricks Code.

    // FIXME: This check is (and should only be) done in the BLE constructor,
    // but it may still pass there since 0 is a valid broadcast channel. That
    // should be fixed by defaulting to None if no broadcast channel is provided.
    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    if (!pbsys_storage_settings_bluetooth_enabled()) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Bluetooth not enabled"));
    }
    #endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE

    // Stop broadcasting if data is None.
    if (data_in == mp_const_none) {
        static pbio_task_t stop_broadcasting_task;
        pbdrv_bluetooth_stop_broadcasting(&stop_broadcasting_task);
        return pb_module_tools_pbio_task_wait_or_await(&stop_broadcasting_task);
    }

    static struct {
        pbdrv_bluetooth_value_t v;
        uint8_t d[5 + OBSERVED_DATA_MAX_SIZE];
    } value;

    // Get either one or several data objects ready for transmission.
    mp_obj_t *objs;
    size_t n_objs;
    size_t index;
    if (pb_obj_is_array(data_in)) {
        index = 0;
        mp_obj_get_array(data_in, &n_objs, &objs);
    } else {
        // Set first type to indicate single object.
        value.v.data[5] = PB_BLE_BROADCAST_DATA_TYPE_SINGLE_OBJECT << 5;
        // The one and only value is included directly after.
        index = 1;
        n_objs = 1;
        objs = &data_in;
    }

    // Encode all objects.
    for (size_t i = 0; i < n_objs; i++) {
        index = pb_module_ble_encode(&value.v.data[5], index, objs[i]);
    }

    value.v.size = index + 5;
    value.v.data[0] = index + 4; // length
    value.v.data[1] = MFG_SPECIFIC;
    pbio_set_uint16_le(&value.v.data[2], LEGO_CID);
    value.v.data[4] = self->broadcast_channel;

    pbdrv_bluetooth_start_broadcasting(self->broadcast_task, &value.v);
    return pb_module_tools_pbio_task_wait_or_await(self->broadcast_task);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_module_ble_broadcast_obj, 1, pb_module_ble_broadcast);

/**
 * Decodes data that was received by the Bluetooth radio.
 *
 * @param [in]      data    Pointer to the start of the advertising data.
 * @param [in,out]  index   When calling, set to the index in @p data to read.
 *                          On return, the value is updated to the next index.
 * @returns                 The decoded value as a Python object.
 * @throws RuntimeError     If the data was invalid and could not be decoded.
 */
static mp_obj_t pb_module_ble_decode(const observed_data_t *data, size_t *index) {
    uint8_t size = data->data[*index] & 0x1F;
    pb_ble_broadcast_data_type_t data_type = data->data[*index] >> 5;

    (*index)++;

    switch (data_type) {
        case PB_BLE_BROADCAST_DATA_TYPE_TRUE:
            assert(size == 0);
            return mp_const_true;
        case PB_BLE_BROADCAST_DATA_TYPE_FALSE:
            assert(size == 0);
            return mp_const_false;
        case PB_BLE_BROADCAST_DATA_TYPE_INT:
            if (size == sizeof(int8_t)) {
                int8_t int8_value = data->data[*index];
                (*index) += sizeof(int8_value);
                return MP_OBJ_NEW_SMALL_INT(int8_value);
            }

            if (size == sizeof(int16_t)) {
                int16_t int16_value = pbio_get_uint16_le(&data->data[*index]);
                (*index) += sizeof(int16_value);
                return MP_OBJ_NEW_SMALL_INT(int16_value);
            }

            if (size == sizeof(int32_t)) {
                int32_t int32_value = pbio_get_uint32_le(&data->data[*index]);
                (*index) += sizeof(int32_value);
                return mp_obj_new_int(int32_value);
            }

            break;

        case PB_BLE_BROADCAST_DATA_TYPE_FLOAT:
            #if MICROPY_FLOAT_IMPL != MICROPY_FLOAT_IMPL_NONE
        {
            union {
                float f;
                uint32_t u;
            } float_value;
            float_value.u = pbio_get_uint32_le(&data->data[*index]);
            (*index) += sizeof(float_value);
            return mp_obj_new_float_from_f(float_value.f);
        }
            #else
            break;
            #endif

        case PB_BLE_BROADCAST_DATA_TYPE_STR: {
            const char *str_data = (void *)&data->data[*index];
            (*index) += size;
            return mp_obj_new_str(str_data, size);
        }

        case PB_BLE_BROADCAST_DATA_TYPE_BYTES: {
            const byte *bytes_data = (void *)&data->data[*index];
            (*index) += size;
            return mp_obj_new_bytes(bytes_data, size);
        }
        case PB_BLE_BROADCAST_DATA_TYPE_SINGLE_OBJECT:
            // Does not contain data by itself, is only used as indicator
            // that the next data is the one and only object.
            break;
    }

    mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("received bad data"));
}

/**
 * Retrieves the last received advertising data.
 *
 * The data in the returned pointer is only consistent until the next PBIO event
 * is processed!
 *
 * @param [in]  channel_in  Python object containing the channel number.
 * @returns                 Pointer to the last received channel data.
 * @throws ValueError       If the channel is out of range.
 * @throws RuntimeError     If the last received data was invalid.
 */
static const observed_data_t *pb_module_ble_get_channel_data(mp_obj_t channel_in) {
    mp_int_t channel = mp_obj_get_int(channel_in);

    observed_data_t *ch_data = lookup_observed_data(channel);

    if (!ch_data) {
        mp_raise_ValueError(MP_ERROR_TEXT("channel not allocated"));
    }

    // Reset the data if it is too old.
    if (mp_hal_ticks_ms() - ch_data->timestamp > OBSERVED_DATA_TIMEOUT_MS) {
        ch_data->size = 0;
        ch_data->rssi = INT8_MIN;
    }

    return ch_data;
}

/**
 * Retrieves the last received advertising data.
 *
 * @param [in]  self_in     The BLE object.
 * @param [in]  channel_in  Python object containing the channel number.
 * @returns                 Python object containing a tuple of decoded data or
 *                          None if no data has been received within
 *                          ::OBSERVED_DATA_TIMEOUT_MS.
 * @throws ValueError       If the channel is out of range.
 * @throws RuntimeError     If the last received data was invalid.
 */
static mp_obj_t pb_module_ble_observe(mp_obj_t self_in, mp_obj_t channel_in) {

    // BEWARE OF DRAGONS: The data returned by pb_module_ble_get_channel_data()
    // is only valid until the next PBIO event is processed, which can happen
    // during any MicroPython function call that allocates memory. So, we have
    // to make a copy of it since we are potentially allocating multiple times
    // in a loop below.
    const observed_data_t ch_data = *pb_module_ble_get_channel_data(channel_in);

    // Have not received data yet or timed out.
    if (ch_data.rssi == INT8_MIN) {
        return mp_const_none;
    }

    // Handle single object.
    if (ch_data.size != 0 && ch_data.data[0] >> 5 == PB_BLE_BROADCAST_DATA_TYPE_SINGLE_OBJECT) {
        size_t value_index = 1;
        return pb_module_ble_decode(&ch_data, &value_index);
    }

    // Objects can be encoded in as little as one byte so we could have up to
    // this many objects received.
    mp_obj_t items[OBSERVED_DATA_MAX_SIZE];

    size_t index = 0;
    size_t i;
    for (i = 0; i < OBSERVED_DATA_MAX_SIZE; i++) {
        if (index >= ch_data.size) {
            break;
        }

        items[i] = pb_module_ble_decode(&ch_data, &index);
    }

    return mp_obj_new_tuple(i, items);
}
static MP_DEFINE_CONST_FUN_OBJ_2(pb_module_ble_observe_obj, pb_module_ble_observe);

/**
 * Retrieves the filtered RSSI signal strength of the given channel.
 *
 * @param [in]  self_in     The BLE object.
 * @param [in]  channel_in  Python object containing the channel number.
 * @returns                 Python object containing the filtered RSSI.
 * @throws ValueError       If the channel is out of range.
 */
static mp_obj_t pb_module_ble_signal_strength(mp_obj_t self_in, mp_obj_t channel_in) {
    const observed_data_t *ch_data = pb_module_ble_get_channel_data(channel_in);
    return mp_obj_new_int(ch_data->rssi);
}
static MP_DEFINE_CONST_FUN_OBJ_2(pb_module_ble_signal_strength_obj, pb_module_ble_signal_strength);

/**
 * Gets the Bluetooth chip frimware version.
 * @param [in]  self_in     The BLE MicroPython object instance.
 * @returns                 MicroPython object containing the firmware version
 *                          as a str.
 */
static mp_obj_t pb_module_ble_version(mp_obj_t self_in) {
    const char *version = pbdrv_bluetooth_get_fw_version();
    return mp_obj_new_str(version, strlen(version));
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_module_ble_version_obj, pb_module_ble_version);

static const mp_rom_map_elem_t common_BLE_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_broadcast), MP_ROM_PTR(&pb_module_ble_broadcast_obj) },
    { MP_ROM_QSTR(MP_QSTR_observe), MP_ROM_PTR(&pb_module_ble_observe_obj) },
    { MP_ROM_QSTR(MP_QSTR_signal_strength), MP_ROM_PTR(&pb_module_ble_signal_strength_obj) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&pb_module_ble_version_obj) },
};
static MP_DEFINE_CONST_DICT(common_BLE_locals_dict, common_BLE_locals_dict_table);

static MP_DEFINE_CONST_OBJ_TYPE(pb_type_BLE,
    MP_QSTR_BLE,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_BLE_locals_dict);

/**
 * Creates a new instance of the BLE class.
 *
 * Do not call this function more than once unless pb_type_ble_start_cleanup() is called first.
 *
 * @param [in]  broadcast_channel_in    (int) The channel number to use for broadcasting.
 * @param [in]  observe_channels_in     (list[int]) A list of channels numbers to observe.
 * @returns                             A newly allocated object.
 * @throws ValueError                   If either parameter contains an out of range channel number.
 */
mp_obj_t pb_type_BLE_new(mp_obj_t broadcast_channel_in, mp_obj_t observe_channels_in) {
    // making the assumption that this is only called once before each pb_type_ble_start_cleanup()
    assert(observed_data == NULL);

    mp_int_t broadcast_channel = mp_obj_get_int(broadcast_channel_in);

    if (broadcast_channel < 0 || broadcast_channel > UINT8_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("broadcast channel must be 0 to 255"));
    }

    mp_int_t num_channels = mp_obj_get_int(mp_obj_len(observe_channels_in));

    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    if (!pbsys_storage_settings_bluetooth_enabled() && (num_channels > 0 || broadcast_channel)) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Bluetooth not enabled"));
    }
    #endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE

    if (num_channels < 0 || num_channels > UINT8_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("len observe channels must be 0 to 255"));
    }

    pb_obj_BLE_t *self = mp_obj_malloc_var(pb_obj_BLE_t, observed_data_t, num_channels, &pb_type_BLE);
    self->broadcast_task = &broadcast_task;
    self->broadcast_channel = broadcast_channel;

    for (mp_int_t i = 0; i < num_channels; i++) {
        mp_int_t channel = mp_obj_get_int(mp_obj_subscr(
            observe_channels_in, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_SENTINEL));

        if (channel < 0 || channel > UINT8_MAX) {
            mp_raise_ValueError(MP_ERROR_TEXT("observe channel must be 0 to 255"));
        }

        self->observed_data[i].channel = channel;
        self->observed_data[i].rssi = INT8_MIN;

        // Suppress stale data by making everything outdated.
        self->observed_data[i].timestamp = mp_hal_ticks_ms() - RSSI_FILTER_WINDOW_MS - OBSERVED_DATA_TIMEOUT_MS;
    }

    // globals for driver callback
    observed_data = self->observed_data;
    num_observed_data = num_channels;

    // Start observing.
    if (num_channels > 0) {
        pbio_task_t task;
        pbdrv_bluetooth_start_observing(&task, handle_observe_event);
        pb_module_tools_pbio_task_do_blocking(&task, -1);
    }

    return MP_OBJ_FROM_PTR(self);
}

void pb_type_ble_start_cleanup(void) {
    static pbio_task_t stop_broadcasting_task;
    static pbio_task_t stop_observing_task;
    pbdrv_bluetooth_stop_broadcasting(&stop_broadcasting_task);
    pbdrv_bluetooth_stop_observing(&stop_observing_task);
    observed_data = NULL;
    num_observed_data = 0;
    // Tasks awaited in pybricks de-init.
}

#endif // PYBRICKS_PY_COMMON_BLE
