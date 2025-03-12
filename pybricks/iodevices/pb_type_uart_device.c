// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include "py/mphal.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.iodevices.uart_device class object
typedef struct _pb_type_uart_device_obj_t {
    pb_type_device_obj_base_t device_base;
} pb_type_uart_device_obj_t;

// pybricks.iodevices.UARTDevice.__init__
static mp_obj_t pb_type_uart_device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_REQUIRED(baudrate),
        PB_ARG_DEFAULT_NONE(timeout));

    // Get device, which inits UART port
    pb_type_uart_device_obj_t *self = mp_obj_malloc(pb_type_uart_device_obj_t, type);

    (void)port_in;
    (void)baudrate_in;
    (void)timeout_in;

    return MP_OBJ_FROM_PTR(self);
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

    // Get data and length
    GET_STR_DATA_LEN(data_in, data, data_len);

    (void)self;
    (void)data;
    (void)data_len;

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_uart_device_write_obj, 1, pb_type_uart_device_write);

// pybricks.iodevices.UARTDevice.in_waiting
static mp_obj_t pb_type_uart_device_in_waiting(mp_obj_t self_in) {
    pb_type_uart_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    (void)self;
    return mp_obj_new_int(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_uart_device_in_waiting_obj, pb_type_uart_device_in_waiting);

// pybricks.iodevices.UARTDevice.read
static mp_obj_t pb_type_uart_device_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_uart_device_obj_t, self,
        PB_ARG_DEFAULT_INT(length, 1));

    size_t length = mp_obj_get_int(length_in);
    (void)length;
    (void)self;

    return mp_obj_new_bytes((const uint8_t *)"", 0);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_uart_device_read_obj, 1, pb_type_uart_device_read);

// pybricks.iodevices.UARTDevice.flush
static mp_obj_t pb_type_uart_device_flush(mp_obj_t self_in) {
    pb_type_uart_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    (void)self;
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
