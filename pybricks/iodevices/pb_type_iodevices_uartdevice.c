// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES

#include <pbio/iodev.h>

#include "py/mphal.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_pb/pb_serial.h>

#define UART_MAX_LEN (32 * 1024)

// pybricks.iodevices.UARTDevice class object
typedef struct _iodevices_UARTDevice_obj_t {
    mp_obj_base_t base;
    pb_serial_t *serial;
    mp_int_t timeout;
} iodevices_UARTDevice_obj_t;

// pybricks.iodevices.UARTDevice.__init__
STATIC mp_obj_t iodevices_UARTDevice_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(baudrate),
        PB_ARG_DEFAULT_NONE(timeout));
    iodevices_UARTDevice_obj_t *self = m_new_obj(iodevices_UARTDevice_obj_t);
    self->base.type = (mp_obj_type_t *)otype;

    // Get port number
    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Init UART port
    pb_device_get_device(port, PBIO_IODEV_TYPE_ID_CUSTOM_UART);

    // Initialize serial
    self->timeout = timeout_in == mp_const_none ? -1 : pb_obj_get_int(timeout_in);
    pb_assert(pb_serial_get(&self->serial, port, pb_obj_get_int(baudrate_in)));
    pb_assert(pb_serial_clear(self->serial));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.UARTDevice.write
STATIC mp_obj_t iodevices_UARTDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_UARTDevice_obj_t, self,
        PB_ARG_REQUIRED(data));

    // Assert that data argument are bytes
    if (!(mp_obj_is_str_or_bytes(data_in) || mp_obj_is_type(data_in, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get data and length
    GET_STR_DATA_LEN(data_in, data, data_len);

    // Write data to serial
    pb_assert(pb_serial_write(self->serial, data, data_len));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_UARTDevice_write_obj, 1, iodevices_UARTDevice_write);

// pybricks.iodevices.UARTDevice.waiting
STATIC mp_obj_t iodevices_UARTDevice_waiting(mp_obj_t self_in) {
    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t waiting;
    pb_assert(pb_serial_in_waiting(self->serial, &waiting));
    return mp_obj_new_int(waiting);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_UARTDevice_waiting_obj, iodevices_UARTDevice_waiting);

// pybricks.iodevices.UARTDevice._read_internal
STATIC mp_obj_t iodevices_UARTDevice_read_internal(iodevices_UARTDevice_obj_t *self, size_t len) {

    if (len > UART_MAX_LEN) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // If we don't need to read anything, return empty bytearray
    if (len < 1) {
        uint8_t none = 0;
        return mp_obj_new_bytes(&none, 0);
    }

    // Read data into buffer
    uint8_t *buf = m_malloc(len);
    if (buf == NULL) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    // Initial status
    mp_uint_t time_start = mp_hal_ticks_ms();
    size_t remaining = len;

    // Read up to the timeout
    while (true) {

        // Read and keep track of how much was read
        size_t read_now;
        pb_assert(pb_serial_read(self->serial, &buf[len - remaining], len, &read_now));

        // Decrement remaining count
        remaining -= read_now;

        // If there is nothing remaining, we are done
        if (remaining == 0) {
            break;
        }

        // If we have timed out, let the user know
        if (self->timeout >= 0 && mp_hal_ticks_ms() - time_start > (mp_uint_t)self->timeout) {
            pb_assert(PBIO_ERROR_TIMEDOUT);
        }

        // Wait before looking again
        mp_hal_delay_ms(10);
    }

    // Convert to bytes
    mp_obj_t ret = mp_obj_new_bytes(buf, len);

    // Free internal buffer and return bytes
    m_free(buf, len);
    return ret;
}

// pybricks.iodevices.UARTDevice.read
STATIC mp_obj_t iodevices_UARTDevice_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_UARTDevice_obj_t, self,
        PB_ARG_DEFAULT_INT(length, 1));

    size_t length = mp_obj_get_int(length_in);
    return iodevices_UARTDevice_read_internal(self, length);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_UARTDevice_read_obj, 1, iodevices_UARTDevice_read);

// pybricks.iodevices.UARTDevice.read_all
STATIC mp_obj_t iodevices_UARTDevice_read_all(mp_obj_t self_in) {

    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);

    size_t len;
    pb_assert(pb_serial_in_waiting(self->serial, &len));

    return iodevices_UARTDevice_read_internal(self, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_UARTDevice_read_all_obj, iodevices_UARTDevice_read_all);

// pybricks.iodevices.UARTDevice.clear
STATIC mp_obj_t iodevices_UARTDevice_clear(mp_obj_t self_in) {
    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pb_serial_clear(self->serial));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_UARTDevice_clear_obj, iodevices_UARTDevice_clear);

// dir(pybricks.iodevices.UARTDevice)
STATIC const mp_rom_map_elem_t iodevices_UARTDevice_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),  MP_ROM_PTR(&iodevices_UARTDevice_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_all),  MP_ROM_PTR(&iodevices_UARTDevice_read_all_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),  MP_ROM_PTR(&iodevices_UARTDevice_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_waiting),MP_ROM_PTR(&iodevices_UARTDevice_waiting_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear),MP_ROM_PTR(&iodevices_UARTDevice_clear_obj) },
};
STATIC MP_DEFINE_CONST_DICT(iodevices_UARTDevice_locals_dict, iodevices_UARTDevice_locals_dict_table);

// type(pybricks.iodevices.UARTDevice)
const mp_obj_type_t pb_type_iodevices_UARTDevice = {
    { &mp_type_type },
    .name = MP_QSTR_UARTDevice,
    .make_new = iodevices_UARTDevice_make_new,
    .locals_dict = (mp_obj_dict_t *)&iodevices_UARTDevice_locals_dict,
};

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES
