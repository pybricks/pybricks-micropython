// SPDX-License-Identifier: MIT
// Copyright (c) 2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <stdint.h>

#include <lego_lwp3.h>

#include <pbdrv/bluetooth.h>
#include <pbio/button.h>
#include <pbio/color.h>
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

enum {
    REMOTE_PORT_LEFT_BUTTONS    = 0,
    REMOTE_PORT_RIGHT_BUTTONS   = 1,
    REMOTE_PORT_STATUS_LIGHT    = 52,
    // NB: items below included for completeness - don't seem to work
    // use hub properties instead
    REMOTE_PORT_BATTERY_VOLTAGE = 59,
    REMOTE_PORT_RSSI            = 60,
};

enum {
    REMOTE_BUTTONS_MODE_RCKEY   = 0,
    REMOTE_BUTTONS_MODE_KEYA    = 1,
    REMOTE_BUTTONS_MODE_KEYR    = 2,
    REMOTE_BUTTONS_MODE_KEYD    = 3,
    REMOTE_BUTTONS_MODE_KEYSD   = 4,
};

enum {
    STATUS_LIGHT_MODE_COL_0     = 0,
    STATUS_LIGHT_MODE_RGB_0     = 1,
};

typedef struct {
    pbio_task_t task;
    uint8_t left[3];
    uint8_t right[3];
    uint8_t center;
    pbdrv_bluetooth_scan_and_connect_context_t context;
} pb_remote_t;

STATIC pb_remote_t pb_remote_singleton;

// Handles LEGO Wireless protocol messages from the handset
STATIC void handle_notification(pbdrv_bluetooth_connection_t connection, const uint8_t *value, uint8_t size) {
    pb_remote_t *remote = &pb_remote_singleton;

    if (value[0] == 5 && value[2] == LWP3_MSG_TYPE_HW_NET_CMDS && value[3] == LWP3_HW_NET_CMD_CONNECTION_REQ) {
        // This message is meant for something else, but contains the center button state
        remote->center = value[4];
    } else if (value[0] == 7 && value[2] == LWP3_MSG_TYPE_PORT_VALUE) {
        // This assumes that the handset button ports have already been set to mode KEYSD
        if (value[3] == REMOTE_PORT_LEFT_BUTTONS) {
            memcpy(remote->left, &value[4], 3);
        } else if (value[3] == REMOTE_PORT_RIGHT_BUTTONS) {
            memcpy(remote->right, &value[4], 3);
        }
    }
}

STATIC void remote_assert_connected(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_LWP3)) {
        mp_raise_OSError(MP_ENODEV);
    }
}

STATIC void pb_type_pupdevices_Remote_light_on(void *context, const pbio_color_hsv_t *hsv) {
    pb_remote_t *remote = &pb_remote_singleton;

    remote_assert_connected();

    struct {
        pbdrv_bluetooth_value_t value;
        uint8_t length;
        uint8_t hub;
        uint8_t type;
        uint8_t port;
        uint8_t startup : 4;
        uint8_t completion : 4;
        uint8_t cmd;
        uint8_t mode;
        uint8_t payload[3];
    } __attribute__((packed)) msg = {
        .value.size = 10,
        .length = 10,
        .type = LWP3_MSG_TYPE_OUT_PORT_CMD,
        .port = REMOTE_PORT_STATUS_LIGHT,
        .startup = LWP3_STARTUP_BUFFER,
        .completion = LWP3_COMPLETION_NO_ACTION,
        .cmd = LWP3_OUTPUT_CMD_WRITE_DIRECT_MODE_DATA,
        .mode = STATUS_LIGHT_MODE_RGB_0,
    };

    pbio_color_hsv_to_rgb(hsv, (pbio_color_rgb_t *)msg.payload);

    pbdrv_bluetooth_write_remote(&remote->task, &msg.value);
    pb_wait_task(&remote->task, -1);
}

STATIC void remote_connect(const char *name, mp_int_t timeout) {
    pb_remote_t *remote = &pb_remote_singleton;

    // REVISIT: for now, we only allow a single connection to a LWP3 device.
    if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL_LWP3)) {
        pb_assert(PBIO_ERROR_BUSY);
    }

    // needed to ensure that no buttons are "pressed" after reconnecting since
    // we are using static memory
    memset(remote, 0, sizeof(*remote));

    remote->context.hub_kind = LWP3_HUB_KIND_HANDSET;

    if (name) {
        strncpy(remote->context.name, name, sizeof(remote->context.name));
    }

    pbdrv_bluetooth_set_notification_handler(handle_notification);
    pbdrv_bluetooth_scan_and_connect(&remote->task, &remote->context);
    pb_wait_task(&remote->task, timeout);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        struct {
            pbdrv_bluetooth_value_t value;
            uint8_t length;
            uint8_t hub;
            uint8_t type;
            uint8_t port;
            uint8_t mode;
            uint32_t delta_interval;
            uint8_t enable_notifications;
        } __attribute__((packed)) msg = {
            .value.size = 10,
            .length = 10,
            .hub = 0,
            .type = LWP3_MSG_TYPE_PORT_MODE_SETUP,
            .port = REMOTE_PORT_LEFT_BUTTONS,
            .mode = REMOTE_BUTTONS_MODE_KEYSD,
            .delta_interval = 1,
            .enable_notifications = 1,
        };

        // set mode for left buttons

        pbdrv_bluetooth_write_remote(&remote->task, &msg.value);
        pb_wait_task(&remote->task, -1);

        // set mode for right buttons

        msg.port = REMOTE_PORT_RIGHT_BUTTONS;
        pbdrv_bluetooth_write_remote(&remote->task, &msg.value);
        pb_wait_task(&remote->task, -1);

        // set status light to RGB mode

        msg.port = REMOTE_PORT_STATUS_LIGHT;
        msg.mode = STATUS_LIGHT_MODE_RGB_0;
        msg.enable_notifications = 0;
        pbdrv_bluetooth_write_remote(&remote->task, &msg.value);
        pb_wait_task(&remote->task, -1);

        // REVISIT: Could possibly use system color here to make remote match
        // hub status light. For now, the system color is hard-coded to blue.
        pbio_color_hsv_t hsv;
        pbio_color_to_hsv(PBIO_COLOR_BLUE, &hsv);
        pb_type_pupdevices_Remote_light_on(NULL, &hsv);

        nlr_pop();
    } else {
        // disconnect if any setup task failed
        pbdrv_bluetooth_disconnect_remote();
        nlr_jump(nlr.ret_val);
    }
}

void pb_type_Remote_cleanup(void) {
    pbdrv_bluetooth_disconnect_remote();
}

STATIC pbio_error_t remote_button_is_pressed(pbio_button_flags_t *pressed) {
    pb_remote_t *remote = &pb_remote_singleton;

    remote_assert_connected();

    *pressed = 0;

    if (remote->left[0]) {
        *pressed |= PBIO_BUTTON_LEFT_UP;
    }
    if (remote->left[1]) {
        *pressed |= PBIO_BUTTON_LEFT;
    }
    if (remote->left[2]) {
        *pressed |= PBIO_BUTTON_LEFT_DOWN;
    }
    if (remote->right[0]) {
        *pressed |= PBIO_BUTTON_RIGHT_UP;
    }
    if (remote->right[1]) {
        *pressed |= PBIO_BUTTON_RIGHT;
    }
    if (remote->right[2]) {
        *pressed |= PBIO_BUTTON_RIGHT_DOWN;
    }
    if (remote->center) {
        *pressed |= PBIO_BUTTON_CENTER;
    }
    return PBIO_SUCCESS;
}

typedef struct _pb_type_pupdevices_Remote_obj_t {
    mp_obj_base_t base;
    mp_obj_t buttons;
    mp_obj_t light;
} pb_type_pupdevices_Remote_obj_t;

STATIC const pb_obj_enum_member_t *remote_buttons[] = {
    &pb_Button_LEFT_MINUS_obj,
    &pb_Button_RIGHT_MINUS_obj,
    &pb_Button_LEFT_obj,
    &pb_Button_CENTER_obj,
    &pb_Button_RIGHT_obj,
    &pb_Button_LEFT_PLUS_obj,
    &pb_Button_RIGHT_PLUS_obj
};

STATIC mp_obj_t pb_type_pupdevices_Remote_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_type_pupdevices_Remote_obj_t *self = m_new_obj(pb_type_pupdevices_Remote_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    const char *name = name_in == mp_const_none ? NULL : mp_obj_str_get_str(name_in);
    mp_int_t timeout = timeout_in == mp_const_none ? -1 : pb_obj_get_positive_int(timeout_in);
    remote_connect(name, timeout);

    self->buttons = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(remote_buttons), remote_buttons, remote_button_is_pressed);
    self->light = pb_type_ColorLight_external_obj_new(NULL, pb_type_pupdevices_Remote_light_on);
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t remote_name(size_t n_args, const mp_obj_t *args) {
    pb_remote_t *remote = &pb_remote_singleton;

    remote_assert_connected();

    if (n_args == 2) {
        size_t len;
        const char *name = mp_obj_str_get_data(args[1], &len);

        if (len == 0 || len > LWP3_MAX_HUB_PROPERTY_NAME_SIZE) {
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
        pbdrv_bluetooth_write_remote(&remote->task, &msg.value);
        pb_wait_task(&remote->task, -1);

        // assuming write was successful instead of reading back from the handset
        memcpy(remote->context.name, name, len);
        remote->context.name[len] = 0;

        return mp_const_none;
    }

    return mp_obj_new_str(remote->context.name, strlen(remote->context.name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(remote_name_obj, 1, 2, remote_name);

STATIC const mp_rom_map_elem_t attribute_table[] = {
    PB_DEFINE_CONST_ATTR_RO(pb_type_pupdevices_Remote_obj_t, MP_QSTR_buttons, buttons),
    PB_DEFINE_CONST_ATTR_RO(pb_type_pupdevices_Remote_obj_t, MP_QSTR_light, light),
};
STATIC MP_DEFINE_CONST_DICT(attribute_dict, attribute_table);

STATIC const mp_rom_map_elem_t pb_type_pupdevices_Remote_locals_dict_table[] = {
    PB_ATTRIBUTE_TABLE(attribute_dict),
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&remote_name_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_pupdevices_Remote_locals_dict, pb_type_pupdevices_Remote_locals_dict_table);

const mp_obj_type_t pb_type_pupdevices_Remote = {
    { &mp_type_type },
    .name = MP_QSTR_Remote,
    .make_new = pb_type_pupdevices_Remote_make_new,
    .attr = pb_attribute_handler,
    .locals_dict = (mp_obj_dict_t *)&pb_type_pupdevices_Remote_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
