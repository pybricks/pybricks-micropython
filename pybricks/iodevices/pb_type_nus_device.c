// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES_NUS_DEVICE

#include <stdint.h>
#include <string.h>

#include "py/objstr.h"
#include "py/runtime.h"

#include <pbdrv/bluetooth.h>

#include <pybricks/iodevices/iodevices.h>
#include <pybricks/tools/pb_type_async.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct _pb_type_nus_device_obj_t {
    mp_obj_base_t base;
    mp_obj_t write_obj;
    pb_type_async_t *write_iter;
    uint8_t rx_len;
    uint8_t rx[PBDRV_BLUETOOTH_MAX_CHAR_SIZE];
} pb_type_nus_device_obj_t;

static pb_type_nus_device_obj_t *nus_instance;

static void handle_nus_rx(const uint8_t *data, uint32_t size) {
    if (!nus_instance) {
        return;
    }
    if (size > PBDRV_BLUETOOTH_MAX_CHAR_SIZE) {
        size = PBDRV_BLUETOOTH_MAX_CHAR_SIZE;
    }
    memcpy(nus_instance->rx, data, size);
    nus_instance->rx_len = size;
}

static mp_obj_t pb_type_nus_device_close(mp_obj_t self_in) {
    if (nus_instance == MP_OBJ_TO_PTR(self_in)) {
        pbdrv_bluetooth_set_nus_receive_handler(NULL);
        nus_instance = NULL;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_nus_device_close_obj, pb_type_nus_device_close);

static mp_obj_t pb_type_nus_device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    if (nus_instance) {
        pb_assert(PBIO_ERROR_BUSY);
    }

    pb_type_nus_device_obj_t *self = mp_obj_malloc_with_finaliser(pb_type_nus_device_obj_t, type);
    self->write_iter = NULL;
    self->write_obj = MP_OBJ_NULL;
    self->rx_len = 0;

    nus_instance = self;
    pbdrv_bluetooth_set_nus_receive_handler(handle_nus_rx);

    return MP_OBJ_FROM_PTR(self);
}

static void pb_type_nus_device_raise_not_connected(void) {
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT(
        "\n\n"
        "No connected device has NUS TX notifications enabled:\n"
        "--> Enable Notify on the TX characteristic (nRF Connect or similar).\n"
        "--> Check nd.is_connected() before write().\n"
        "\n"));
}

static pbio_error_t pb_type_nus_device_write_iter_once(pbio_os_state_t *state, mp_obj_t self_in) {
    pb_type_nus_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GET_STR_DATA_LEN(self->write_obj, data, data_len);
    if (data_len > UINT16_MAX) {
        return PBIO_ERROR_INVALID_ARG;
    }
    pbio_error_t err = pbdrv_bluetooth_send_nus_notification(state, data, (uint16_t)data_len);
    if (err == PBIO_ERROR_INVALID_OP) {
        pb_type_nus_device_raise_not_connected();
    }
    return err;
}

static mp_obj_t pb_type_nus_device_write_return_map(mp_obj_t self_in) {
    pb_type_nus_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->write_obj = MP_OBJ_NULL;
    return mp_const_none;
}

static mp_obj_t pb_type_nus_device_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_nus_device_obj_t, self,
        PB_ARG_REQUIRED(data));

    if (!(mp_obj_is_str_or_bytes(data_in) || mp_obj_is_type(data_in, &mp_type_bytearray))) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    if (!pbdrv_bluetooth_nus_is_connected()) {
        pb_type_nus_device_raise_not_connected();
    }

    self->write_obj = data_in;

    pb_type_async_t config = {
        .iter_once = pb_type_nus_device_write_iter_once,
        .parent_obj = MP_OBJ_FROM_PTR(self),
        .return_map = pb_type_nus_device_write_return_map,
        .state = 0,
    };
    return pb_type_async_wait_or_await(&config, &self->write_iter, true);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_nus_device_write_obj, 1, pb_type_nus_device_write);

static mp_obj_t pb_type_nus_device_read_all(mp_obj_t self_in) {
    pb_type_nus_device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->rx_len == 0) {
        return mp_const_empty_bytes;
    }

    mp_obj_t result = mp_obj_new_bytes(self->rx, self->rx_len);
    self->rx_len = 0;
    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_nus_device_read_all_obj, pb_type_nus_device_read_all);

static mp_obj_t pb_type_nus_device_is_connected(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(pbdrv_bluetooth_nus_is_connected());
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_nus_device_is_connected_obj, pb_type_nus_device_is_connected);

static const mp_rom_map_elem_t pb_type_nus_device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__),      MP_ROM_PTR(&pb_type_nus_device_close_obj)        },
    { MP_ROM_QSTR(MP_QSTR_read_all),     MP_ROM_PTR(&pb_type_nus_device_read_all_obj)     },
    { MP_ROM_QSTR(MP_QSTR_write),        MP_ROM_PTR(&pb_type_nus_device_write_obj)        },
    { MP_ROM_QSTR(MP_QSTR_is_connected), MP_ROM_PTR(&pb_type_nus_device_is_connected_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_nus_device_locals_dict, pb_type_nus_device_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_nus_device,
    MP_QSTR_NUSDevice,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_nus_device_make_new,
    locals_dict, &pb_type_nus_device_locals_dict);

#endif // PYBRICKS_PY_IODEVICES_NUS_DEVICE
