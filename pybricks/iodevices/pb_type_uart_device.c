// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include "py/mphal.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include <contiki.h>

#include <pbdrv/uart.h>
#include <pbio/port_interface.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.iodevices.uart_device class object
typedef struct _pb_type_uart_device_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
    pbdrv_uart_dev_t *uart_dev;
    uint32_t timeout;
    struct pt write_pt;
    mp_obj_t write_obj;
    mp_obj_t write_awaitables;
    struct pt read_pt;
    mp_obj_t read_obj;
    mp_obj_t read_awaitables;
} pb_type_uart_device_obj_t;

// pybricks.iodevices.UARTDevice.__init__
static mp_obj_t pb_type_uart_device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(baudrate),
        PB_ARG_DEFAULT_NONE(timeout));

    // Get device, which inits UART port
    pb_type_uart_device_obj_t *self = mp_obj_malloc(pb_type_uart_device_obj_t, type);

    (void)baudrate_in;

    if (timeout_in == mp_const_none || pb_obj_get_int(timeout_in) < 0) {
        self->timeout = 0;
    } else {
        self->timeout = pb_obj_get_int(timeout_in);
    }

    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(pbio_port_get_port(port_id, &self->port));
    pbio_port_set_mode(self->port, PBIO_PORT_MODE_UART);
    pb_assert(pbio_port_get_uart_dev(self->port, &self->uart_dev));

    // List of awaitables associated with reading and writing.
    self->write_awaitables = mp_obj_new_list(0, NULL);
    self->read_awaitables = mp_obj_new_list(0, NULL);

    return MP_OBJ_FROM_PTR(self);
}

static bool pb_type_uart_device_write_test_completion(mp_obj_t self_in, uint32_t end_time) {
    pb_type_uart_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(self->write_obj, data, data_len);

    // Set current process to port process even though user code does not deal
    // with any processes. This ensures that any references set on etimer don't
    // accidentally touch other processes.
    pbio_port_select_process(self->port);

    // Runs one iteration of the write protothread.
    pbio_error_t err;
    bool awaiting = PT_SCHEDULE(pbdrv_uart_write(&self->read_pt, self->uart_dev, (uint8_t *)data, data_len, self->timeout, &err));
    if (awaiting) {
        return false;
    }

    // Complete or stopped, so allow written object to be garbage collected.
    self->write_obj = mp_const_none;

    // Either completed or timed out, so assert it.
    pb_assert(err);
    return true;
}

// pybricks.iodevices.UARTDevice.write
static mp_obj_t pb_type_uart_device_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_uart_device_obj_t, self,
        PB_ARG_REQUIRED(data));

    // Assert that data argument are bytes
    if (!(mp_obj_is_str_or_bytes(data_in) || mp_obj_is_type(data_in, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    PT_INIT(&self->write_pt);

    // Prevents this object from being garbage collected while the write is in progress.
    self->write_obj = data_in;

    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(self),
        self->write_awaitables,
        pb_type_awaitable_end_time_none,
        pb_type_uart_device_write_test_completion,
        pb_type_awaitable_return_none,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_RAISE_ON_BUSY);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_uart_device_write_obj, 1, pb_type_uart_device_write);

// pybricks.iodevices.UARTDevice.in_waiting
static mp_obj_t pb_type_uart_device_in_waiting(mp_obj_t self_in) {
    pb_type_uart_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    (void)self;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    return mp_obj_new_int(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_uart_device_in_waiting_obj, pb_type_uart_device_in_waiting);

static bool pb_type_uart_device_read_test_completion(mp_obj_t self_in, uint32_t end_time) {
    pb_type_uart_device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Set current process to port process even though user code does not deal
    // with any processes. This ensures that any references set on etimer don't
    // accidentally touch other processes.
    pbio_port_select_process(self->port);

    mp_obj_str_t *str = MP_OBJ_TO_PTR(self->read_obj);

    // Runs one iteration of the read protothread.
    pbio_error_t err;
    bool awaiting = PT_SCHEDULE(pbdrv_uart_read(&self->write_pt, self->uart_dev, (uint8_t *)str->data, str->len, self->timeout, &err));
    if (awaiting) {
        return false;
    }

    // Either completed or timed out, so assert it.
    pb_assert(err);
    return true;
}

static mp_obj_t pb_type_uart_device_read_return_value(mp_obj_t self_in) {
    pb_type_uart_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t ret = self->read_obj;
    self->read_obj = mp_const_none;
    return ret;
}

// pybricks.iodevices.UARTDevice.read
static mp_obj_t pb_type_uart_device_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_uart_device_obj_t, self,
        PB_ARG_DEFAULT_INT(length, 1));

    // Creates zeroed bytes object of given length, by calling Python function bytes(length).
    mp_obj_t args[] = { length_in };
    self->read_obj = MP_OBJ_TYPE_GET_SLOT(&mp_type_bytes, make_new)((mp_obj_t)&mp_type_bytes, MP_ARRAY_SIZE(args), 0, args);

    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(self),
        self->read_awaitables,
        pb_type_awaitable_end_time_none,
        pb_type_uart_device_read_test_completion,
        pb_type_uart_device_read_return_value,
        pb_type_awaitable_cancel_none,
        PB_TYPE_AWAITABLE_OPT_RAISE_ON_BUSY);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_uart_device_read_obj, 1, pb_type_uart_device_read);

// pybricks.iodevices.UARTDevice.flush
static mp_obj_t pb_type_uart_device_flush(mp_obj_t self_in) {
    pb_type_uart_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbdrv_uart_flush(self->uart_dev);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_uart_device_flush_obj, pb_type_uart_device_flush);

// dir(pybricks.iodevices.uart_device)
static const mp_rom_map_elem_t pb_type_uart_device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),       MP_ROM_PTR(&pb_type_uart_device_read_obj)       },
    { MP_ROM_QSTR(MP_QSTR_write),      MP_ROM_PTR(&pb_type_uart_device_write_obj)      },
    { MP_ROM_QSTR(MP_QSTR_in_waiting), MP_ROM_PTR(&pb_type_uart_device_in_waiting_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush),      MP_ROM_PTR(&pb_type_uart_device_flush_obj)      },
};
static MP_DEFINE_CONST_DICT(pb_type_uart_device_locals_dict, pb_type_uart_device_locals_dict_table);

// type(pybricks.iodevices.uart_device)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_uart_device,
    MP_QSTR_uart_device,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_uart_device_make_new,
    locals_dict, &pb_type_uart_device_locals_dict);

#endif // PYBRICKS_PY_IODEVICES
