// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS_APP_DATA

#include <string.h>

#include <pbsys/command.h>
#include <pbdrv/bluetooth.h>

#include "py/mphal.h"
#include "py/objstr.h"

#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_awaitable.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct _pb_type_app_data_obj_t {
    mp_obj_base_t base;
    pbio_task_t tx_task;
    pbdrv_bluetooth_send_context_t tx_context;
    mp_obj_t rx_format;
    mp_obj_str_t rx_bytes_obj;
    uint8_t tx_buffer[20]; // REVISIT: Could be the negotiated MTU - 3 for better throughput https://github.com/pybricks/support/issues/1727
    uint8_t rx_buffer[] __attribute__((aligned(4)));
} pb_type_app_data_obj_t;

// pointer to dynamically allocated app_data singleton for driver callback.
static pb_type_app_data_obj_t *app_data_instance;

static pbio_error_t handle_incoming_app_data(uint16_t offset, uint32_t size, const uint8_t *data) {
    // Can't write if rx_buffer does not exist or isn't big enough.
    if (!app_data_instance || offset + size > app_data_instance->rx_bytes_obj.len) {
        return PBIO_ERROR_INVALID_ARG;
    }
    memcpy(app_data_instance->rx_buffer + offset, data, size);
    return PBIO_SUCCESS;
}

STATIC mp_obj_t pb_type_app_data_get_bytes(mp_obj_t self_in) {
    pb_type_app_data_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Don't return internal bytes object but make a copy so the user bytes
    // object is constant as would be expected. Revisit: enable and return
    // a memoryview, especially if using large buffers.
    return mp_obj_new_bytes(self->rx_bytes_obj.data, self->rx_bytes_obj.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_app_data_get_bytes_obj, pb_type_app_data_get_bytes);

STATIC mp_obj_t pb_type_app_data_get_values(mp_obj_t self_in) {

    // Implementation in MicroPython is static, so import from ustruct.unpack.
    mp_obj_t ustruct_unpack = pb_function_import_helper(MP_QSTR_ustruct, MP_QSTR_unpack);

    // Host (sender) is responsible for making sure that each individual
    // value remains valid, i.e. is written in a single chunk, since the
    // following may allocate, and thus be updated between unpacking values.
    pb_type_app_data_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_call_function_2(ustruct_unpack, self->rx_format, MP_OBJ_FROM_PTR(&self->rx_bytes_obj));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_app_data_get_values_obj, pb_type_app_data_get_values);

STATIC mp_obj_t pb_type_app_data_write_bytes(mp_obj_t self_in, mp_obj_t data_in) {
    pb_type_app_data_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Copy data to local buffer. Needs to remain valid while sending.
    size_t len;
    const char *data = mp_obj_str_get_data(data_in, &len);

    if (len > sizeof(self->tx_buffer) - 1) {
        mp_raise_msg_varg(&mp_type_ValueError,
            MP_ERROR_TEXT("Cannot send more than %d bytes\n"), sizeof(self->tx_buffer) - 1);
    }

    memcpy(self->tx_buffer + 1, data, len);
    self->tx_context.size = len + 1;

    pbdrv_bluetooth_send_queued(&self->tx_task, &self->tx_context);
    return pb_module_tools_pbio_task_wait_or_await(&self->tx_task);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pb_type_app_data_write_bytes_obj, pb_type_app_data_write_bytes);

STATIC mp_obj_t pb_type_app_data_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(rx_format));

    // Use ustruct.calcsize to parse user rx_format for size.
    mp_obj_t ustruct_calcsize = pb_function_import_helper(MP_QSTR_ustruct, MP_QSTR_calcsize);
    size_t size = mp_obj_get_int(mp_call_function_1(ustruct_calcsize, rx_format_in));

    // Can only create one instance for now.
    if (app_data_instance) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("host rx_buffer already allocated"));
    }

    // Use finalizer so we can deactivate the data callback when rx_buffer is garbage collected.
    app_data_instance = m_new_obj_var_with_finaliser(pb_type_app_data_obj_t, uint8_t, size);
    app_data_instance->base.type = type;
    app_data_instance->rx_format = rx_format_in;

    // Keep rx_buffer in bytes object rx_format for compatibility with unpack.
    app_data_instance->rx_bytes_obj.base.type = &mp_type_bytes;
    app_data_instance->rx_bytes_obj.len = size;
    app_data_instance->rx_bytes_obj.data = app_data_instance->rx_buffer;

    // Activate callback now that we have allocated the rx_buffer.
    pbsys_command_set_write_app_data_callback(handle_incoming_app_data);

    // Prepare tx context. Only the length and data is variable.
    app_data_instance->tx_context.done = NULL;
    app_data_instance->tx_context.connection = PBDRV_BLUETOOTH_CONNECTION_PYBRICKS;
    app_data_instance->tx_context.data = app_data_instance->tx_buffer;
    app_data_instance->tx_buffer[0] = PBIO_PYBRICKS_EVENT_WRITE_APP_DATA;

    return MP_OBJ_FROM_PTR(app_data_instance);
}

mp_obj_t pb_type_app_data_close(mp_obj_t stream) {
    if (app_data_instance) {
        pbsys_command_set_write_app_data_callback(NULL);
        app_data_instance = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_app_data_close_obj, pb_type_app_data_close);

STATIC const mp_rom_map_elem_t pb_type_app_data_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__),      MP_ROM_PTR(&pb_type_app_data_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_close),        MP_ROM_PTR(&pb_type_app_data_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_bytes),    MP_ROM_PTR(&pb_type_app_data_get_bytes_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_values),   MP_ROM_PTR(&pb_type_app_data_get_values_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_bytes),    MP_ROM_PTR(&pb_type_app_data_write_bytes_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_app_data_locals_dict, pb_type_app_data_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_app_data,
    MP_QSTR_AppData,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_app_data_make_new,
    locals_dict, &pb_type_app_data_locals_dict);

#endif // PYBRICKS_PY_TOOLS_APP_DATA
