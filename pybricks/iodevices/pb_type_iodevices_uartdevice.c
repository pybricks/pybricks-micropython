// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

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

#define UART_MAX_LEN (32 * 1024)

#include <contiki.h>

#include <pbdrv/serial.h>

// FIXME: Drop pbio separation and consolidate into class below
typedef struct _pbio_serial_t {
    pbdrv_serial_t *dev;
    int32_t timeout;
    bool busy;
    unsigned long time_start;
    size_t remaining;
} pbio_serial_t;

pbio_serial_t serials[PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT - PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT + 1];

pbio_error_t pbio_serial_get(pbio_serial_t **_ser, pbio_port_t port, uint32_t baudrate, int32_t timeout) {

    if (port < PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT || port > PBDRV_CONFIG_IOPORT_LPF2_LAST_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }

    pbio_serial_t *ser = &serials[port - PBDRV_CONFIG_IOPORT_LPF2_FIRST_PORT];

    pbio_error_t err = pbdrv_serial_get(&ser->dev, port, baudrate);

    if (err != PBIO_SUCCESS) {
        return err;
    }

    ser->timeout = timeout >= 0 ? timeout : -1;

    *_ser = ser;

    return PBIO_SUCCESS;
}

pbio_error_t pbio_serial_write(pbio_serial_t *ser, const void *buf, size_t count) {
    return pbdrv_serial_write(ser->dev, buf, count);
}

pbio_error_t pbio_serial_in_waiting(pbio_serial_t *ser, size_t *waiting) {
    return pbdrv_serial_in_waiting(ser->dev, waiting);
}

static pbio_error_t pbio_serial_read_start(pbio_serial_t *ser, size_t count) {
    // Already started, so return
    if (ser->busy) {
        return PBIO_SUCCESS;
    }

    // Reset state variables
    ser->busy = true;
    ser->time_start = clock_usecs() / 1000;
    ser->remaining = count;

    return PBIO_SUCCESS;
}

static pbio_error_t pbio_serial_read_stop(pbio_serial_t *ser, pbio_error_t err) {
    // Return to default state
    ser->busy = false;

    // Return the error that was raised on stopping
    return err;
}

pbio_error_t pbio_serial_read(pbio_serial_t *ser, uint8_t *buf, size_t count) {

    pbio_error_t err;

    // If this is called for the first time, init:
    err = pbio_serial_read_start(ser, count);
    if (err != PBIO_SUCCESS) {
        return pbio_serial_read_stop(ser, err);
    }

    // Read and keep track of how much was read
    size_t read_now;
    err = pbdrv_serial_read(ser->dev, &buf[count - ser->remaining], count, &read_now);
    if (err != PBIO_SUCCESS) {
        return pbio_serial_read_stop(ser, err);
    }

    // Decrement remaining count
    ser->remaining -= read_now;

    // If there is nothing remaining, we are done
    if (ser->remaining == 0) {
        return pbio_serial_read_stop(ser, PBIO_SUCCESS);
    }

    // If we have timed out, let the user know
    if (ser->timeout >= 0 && clock_usecs() / 1000 - ser->time_start > (unsigned long)ser->timeout) {
        return pbio_serial_read_stop(ser, PBIO_ERROR_TIMEDOUT);
    }

    // If we are here, we need to call this again
    return PBIO_ERROR_AGAIN;
}

pbio_error_t pbio_serial_clear(pbio_serial_t *ser) {
    return pbdrv_serial_clear(ser->dev);
}

// pybricks.iodevices.UARTDevice class object
typedef struct _iodevices_UARTDevice_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
    pbio_serial_t *serial;
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
    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Init UART port
    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_CUSTOM_UART);

    // Get and init serial
    pb_assert(pbio_serial_get(
        &self->serial,
        port_num,
        pb_obj_get_int(baudrate),
        timeout == mp_const_none ? -1 : pb_obj_get_int(timeout)
        ));
    pb_assert(pbio_serial_clear(self->serial));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.UARTDevice.write
STATIC mp_obj_t iodevices_UARTDevice_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        iodevices_UARTDevice_obj_t, self,
        PB_ARG_REQUIRED(data));

    // Assert that data argument are bytes
    if (!(mp_obj_is_str_or_bytes(data) || mp_obj_is_type(data, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get data and length
    GET_STR_DATA_LEN(data, bytes, len);

    // Write data to serial
    pb_assert(pbio_serial_write(self->serial, bytes, len));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_UARTDevice_write_obj, 1, iodevices_UARTDevice_write);

// pybricks.iodevices.UARTDevice.waiting
STATIC mp_obj_t iodevices_UARTDevice_waiting(mp_obj_t self_in) {
    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t waiting;
    pb_assert(pbio_serial_in_waiting(self->serial, &waiting));
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

    // Read up to the timeout
    pbio_error_t err;
    while ((err = pbio_serial_read(self->serial, buf, len)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(10);
    }
    // Raise io/timeout error if needed.
    pb_assert(err);

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

    size_t len = mp_obj_get_int(length);
    return iodevices_UARTDevice_read_internal(self, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(iodevices_UARTDevice_read_obj, 1, iodevices_UARTDevice_read);

// pybricks.iodevices.UARTDevice.read_all
STATIC mp_obj_t iodevices_UARTDevice_read_all(mp_obj_t self_in) {

    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);

    size_t len;
    pb_assert(pbio_serial_in_waiting(self->serial, &len));

    return iodevices_UARTDevice_read_internal(self, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_UARTDevice_read_all_obj, iodevices_UARTDevice_read_all);

// pybricks.iodevices.UARTDevice.clear
STATIC mp_obj_t iodevices_UARTDevice_clear(mp_obj_t self_in) {
    iodevices_UARTDevice_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_serial_clear(self->serial));
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

#endif // PYBRICKS_PY_IODEVICES
