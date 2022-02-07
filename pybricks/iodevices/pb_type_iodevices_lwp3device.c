// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_PUPDEVICES

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <lego_lwp3.h>

#include <pbdrv/bluetooth.h>
#include <pbio/error.h>
#include <pbio/task.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_pb/pb_task.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"

// TODO: most of the functions in this class can be shared with the Remote class

#define LWP3_HEADER_SIZE 3

// MTU is assumed to be 23, not the actual negotiated MTU.
// A overhead of 3 yields a max message size of 20 (=23-3)
#define LWP3_MAX_MESSAGE_SIZE 20

typedef struct {
    pbio_task_t task;
    uint8_t buffer[LWP3_MAX_MESSAGE_SIZE];
    bool notification_received;
    pbdrv_bluetooth_scan_and_connect_context_t context;
} pb_lwp3device_t;

STATIC pb_lwp3device_t pb_lwp3device_singleton;

// Handles LEGO Wireless protocol messages from the LWP3 Device
STATIC void handle_notification(pbdrv_bluetooth_connection_t connection, const uint8_t *value, uint8_t size) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    // Each message overwrites the previous received messages
    // Messages will be lost if they are not read fast enough
    memcpy(lwp3device->buffer, &value[0], (size < LWP3_MAX_MESSAGE_SIZE) ? size : LWP3_MAX_MESSAGE_SIZE);

    lwp3device->notification_received = true;
}

STATIC void lwp3device_connect(const uint8_t hub_kind, const char *name, mp_int_t timeout) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    // REVISIT: for now, we only allow a single connection to a LWP3 device.
    if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_LWP3)) {
        pb_assert(PBIO_ERROR_BUSY);
    }

    // clear memory after reconnect to empty buffer
    // we are using static memory
    memset(lwp3device, 0, sizeof(*lwp3device));

    lwp3device->context.hub_kind = hub_kind;

    if (name) {
        strncpy(lwp3device->context.name, name, sizeof(lwp3device->context.name));
    }

    pbdrv_bluetooth_set_notification_handler(handle_notification);
    pbdrv_bluetooth_scan_and_connect(&lwp3device->task, &lwp3device->context);
    pb_wait_task(&lwp3device->task, timeout);
}

STATIC void lwp3device_assert_connected(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_LWP3)) {
        mp_raise_OSError(MP_ENODEV);
    }
}

typedef struct _pb_type_iodevices_LWP3Device_obj_t {
    mp_obj_base_t base;
} pb_type_iodevices_LWP3Device_obj_t;

STATIC mp_obj_t pb_type_iodevices_LWP3Device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(hub_kind),
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_type_iodevices_LWP3Device_obj_t *self = m_new_obj(pb_type_iodevices_LWP3Device_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    uint8_t hub_kind = pb_obj_get_positive_int(hub_kind_in);

    const char *name = name_in == mp_const_none ? NULL : mp_obj_str_get_str(name_in);
    mp_int_t timeout = timeout_in == mp_const_none ? -1 : pb_obj_get_positive_int(timeout_in);
    lwp3device_connect(hub_kind, name, timeout);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t lwp3device_name(size_t n_args, const mp_obj_t *args) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    lwp3device_assert_connected();

    if (n_args == 2) {
        size_t len;
        const char *name = mp_obj_str_get_data(args[1], &len);

        if (len == 0 || len > LWP3_MAX_MESSAGE_SIZE) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad name length"));
        }

        struct {
            pbdrv_bluetooth_value_t value;
            uint8_t length;
            uint8_t hub;
            uint8_t type;
            uint8_t property;
            uint8_t operation;
            char payload[LWP3_MAX_HUB_PROPERTY_NAME_SIZE];
        } __attribute__((packed)) msg;

        msg.value.size = msg.length = len + 5;
        msg.hub = 0;
        msg.type = LWP3_MSG_TYPE_HUB_PROPERTIES;
        msg.property = LWP3_HUB_PROPERTY_NAME;
        msg.operation = LWP3_HUB_PROPERTY_OP_SET;
        memcpy(msg.payload, name, len);

        // NB: operation is not cancelable, so timeout is not used
        pbdrv_bluetooth_write_remote(&lwp3device->task, &msg.value);
        pb_wait_task(&lwp3device->task, -1);

        // assuming write was successful instead of reading back from the handset
        memcpy(lwp3device->context.name, name, len);
        lwp3device->context.name[len] = 0;

        return mp_const_none;
    }

    return mp_obj_new_str(lwp3device->context.name, strlen(lwp3device->context.name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lwp3device_name_obj, 1, 2, lwp3device_name);

STATIC mp_obj_t lwp3device_write(mp_obj_t self_in, mp_obj_t buf_in) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    lwp3device_assert_connected();

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len < LWP3_HEADER_SIZE || bufinfo.len > LWP3_MAX_MESSAGE_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("bad length"));
    }
    if (((uint8_t *)bufinfo.buf)[0] != bufinfo.len) {
        mp_raise_ValueError(MP_ERROR_TEXT("length in header wrong"));
    }

    struct {
        pbdrv_bluetooth_value_t value;
        char payload[LWP3_MAX_MESSAGE_SIZE];
    } __attribute__((packed)) msg = {
        .value.size = bufinfo.len,
    };
    memcpy(msg.payload, bufinfo.buf, bufinfo.len);

    pbdrv_bluetooth_write_remote(&lwp3device->task, &msg.value);
    pb_wait_task(&lwp3device->task, -1);

    return MP_OBJ_NEW_SMALL_INT(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(lwp3device_write_obj, lwp3device_write);

STATIC mp_obj_t lwp3device_read(mp_obj_t self_in) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    // wait until a notification is received
    for (;;) {
        lwp3device_assert_connected();

        if (lwp3device->notification_received) {
            lwp3device->notification_received = false;
            break;
        }

        MICROPY_EVENT_POLL_HOOK
    }

    size_t len = lwp3device->buffer[0];

    if (len < LWP3_HEADER_SIZE || len > LWP3_MAX_MESSAGE_SIZE) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("bad data"));
    }

    return mp_obj_new_bytes(lwp3device->buffer, len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(lwp3device_read_obj, lwp3device_read);

STATIC const mp_rom_map_elem_t pb_type_iodevices_LWP3Device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&lwp3device_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&lwp3device_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&lwp3device_read_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_iodevices_LWP3Device_locals_dict, pb_type_iodevices_LWP3Device_locals_dict_table);

const mp_obj_type_t pb_type_iodevices_LWP3Device = {
    { &mp_type_type },
    .name = MP_QSTR_LWP3Device,
    .make_new = pb_type_iodevices_LWP3Device_make_new,
    .locals_dict = (mp_obj_dict_t *)&pb_type_iodevices_LWP3Device_locals_dict,
};

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_PUPDEVICES
