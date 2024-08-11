// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2024 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES_REMOTE

#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>

#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/task.h>

#include <pbsys/config.h>
#include <pbsys/storage_settings.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/tools.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mperrno.h"

#define LWP3_HEADER_SIZE 3

// MTU is assumed to be 23, not the actual negotiated MTU.
// A overhead of 3 yields a max message size of 20 (=23-3)
#define LWP3_MAX_MESSAGE_SIZE 20

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

/**
 * LEGO Wireless Protocol v3 Hub Service UUID.
 *
 * 00001623-1212-EFDE-1623-785FEABCD123
 */
static const uint8_t pbio_lwp3_hub_service_uuid[] = {
    0x00, 0x00, 0x16, 0x23, 0x12, 0x12, 0xEF, 0xDE,
    0x16, 0x23, 0x78, 0x5F, 0xEA, 0xBC, 0xD1, 0x23,
};

/**
 * LEGO Wireless Protocol v3 Hub Characteristic UUID.
 *
 * 00001624-1212-EFDE-1623-785FEABCD123
 */
static pbdrv_bluetooth_peripheral_char_t pb_lwp3device_char = {
    .handle = 0, // Will be set during discovery.
    .properties = 0,
    .uuid16 = 0,
    .uuid128 = {
        0x00, 0x00, 0x16, 0x24, 0x12, 0x12, 0xEF, 0xDE,
        0x16, 0x23, 0x78, 0x5F, 0xEA, 0xBC, 0xD1, 0x23,
    },
    .request_notification = true,
};

typedef struct {
    pbio_task_t task;
    #if PYBRICKS_PY_IODEVICES
    uint8_t buffer[LWP3_MAX_MESSAGE_SIZE];
    bool notification_received;
    #endif // PYBRICKS_PY_IODEVICES
    uint8_t left[3];
    uint8_t right[3];
    uint8_t center;
    lwp3_hub_kind_t hub_kind;
    // Name used to filter advertisements and responses.
    // Also used as the name of the device when setting the name, since this
    // is not updated in the driver until the next time it connects.
    char name[LWP3_MAX_HUB_PROPERTY_NAME_SIZE + 1];
} pb_lwp3device_t;

static pb_lwp3device_t pb_lwp3device_singleton;

// Handles LEGO Wireless protocol messages from the LWP3 Device.
static pbio_pybricks_error_t handle_notification(pbdrv_bluetooth_connection_t connection, const uint8_t *value, uint32_t size) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    #if PYBRICKS_PY_IODEVICES
    // Each message overwrites the previous received messages
    // Messages will be lost if they are not read fast enough
    memcpy(lwp3device->buffer, &value[0], (size < LWP3_MAX_MESSAGE_SIZE) ? size : LWP3_MAX_MESSAGE_SIZE);
    lwp3device->notification_received = true;

    if (lwp3device->hub_kind != LWP3_HUB_KIND_HANDSET) {
        // This is not a handset, so we don't care about button state.
        return PBIO_PYBRICKS_ERROR_OK;
    }
    #endif // PYBRICKS_PY_IODEVICES

    // The LWP3 class is mostly just used for the remote, so do the work
    // to parse the button state here.
    if (value[0] == 5 && value[2] == LWP3_MSG_TYPE_HW_NET_CMDS && value[3] == LWP3_HW_NET_CMD_CONNECTION_REQ) {
        // This message is meant for something else, but contains the center button state
        lwp3device->center = value[4];
    } else if (value[0] == 7 && value[2] == LWP3_MSG_TYPE_PORT_VALUE) {
        // This assumes that the handset button ports have already been set to mode KEYSD
        if (value[3] == REMOTE_PORT_LEFT_BUTTONS) {
            memcpy(lwp3device->left, &value[4], 3);
        } else if (value[3] == REMOTE_PORT_RIGHT_BUTTONS) {
            memcpy(lwp3device->right, &value[4], 3);
        }
    }

    return PBIO_PYBRICKS_ERROR_OK;
}

static pbdrv_bluetooth_ad_match_result_flags_t lwp3_advertisement_matches(uint8_t event_type, const uint8_t *data, const char *name, const uint8_t *addr, const uint8_t *match_addr) {
    pbdrv_bluetooth_ad_match_result_flags_t flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;

    // Whether this looks like a LWP3 advertisement of the correct hub kind.
    if (event_type == PBDRV_BLUETOOTH_AD_TYPE_ADV_IND
        && data[3] == 17 /* length */
        && (data[4] == PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_COMPLETE_LIST
            || data[4] == PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_INCOMPLETE_LIST)
        && pbio_uuid128_reverse_compare(&data[5], pbio_lwp3_hub_service_uuid)
        && data[26] == pb_lwp3device_singleton.hub_kind) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_VALUE;
    }

    // Compare address in advertisement to previously scanned address.
    if (memcmp(addr, match_addr, 6) == 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_ADDRESS;
    }
    return flags;
}

static pbdrv_bluetooth_ad_match_result_flags_t lwp3_advertisement_response_matches(uint8_t event_type, const uint8_t *data, const char *name, const uint8_t *addr, const uint8_t *match_addr) {

    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    pbdrv_bluetooth_ad_match_result_flags_t flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;

    // This is the only value check we do on LWP3 response messages.
    if (event_type == PBDRV_BLUETOOTH_AD_TYPE_SCAN_RSP) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_VALUE;
    }

    // Compare address in response to previously scanned address.
    if (memcmp(addr, match_addr, 6) == 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_ADDRESS;
    }

    // Compare name to user-provided name if given.
    if (lwp3device->name[0] != '\0' && strncmp(name, lwp3device->name, 20) != 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_NAME_FAILED;
    }

    return flags;
}

static void pb_lwp3device_assert_connected(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        mp_raise_OSError(MP_ENODEV);
    }
}

static void pb_lwp3device_connect(const char *name, mp_int_t timeout, lwp3_hub_kind_t hub_kind) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    // REVISIT: for now, we only allow a single connection to a LWP3 device.
    if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        pb_assert(PBIO_ERROR_BUSY);
    }

    // HACK: scan and connect may block sending other Bluetooth messages, so we
    // need to make sure the stdout queue is drained first to avoid unexpected
    // behavior
    mp_hal_stdout_tx_flush();

    // needed to ensure that no buttons are "pressed" after reconnecting since
    // we are using static memory
    memset(lwp3device, 0, sizeof(*lwp3device));

    // Hub kind and name are set to filter advertisements and responses.
    lwp3device->hub_kind = hub_kind;
    if (name) {
        strncpy(lwp3device->name, name, sizeof(lwp3device->name));
    }

    pbdrv_bluetooth_peripheral_scan_and_connect(&lwp3device->task,
        lwp3_advertisement_matches,
        lwp3_advertisement_response_matches,
        handle_notification,
        PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_NONE);
    pb_module_tools_pbio_task_do_blocking(&lwp3device->task, timeout);

    // Copy the name so we can read it back later, and override locally.
    memcpy(lwp3device->name, pbdrv_bluetooth_peripheral_get_name(), sizeof(lwp3device->name));

    // Discover the characteristic and enable notifications.
    pbdrv_bluetooth_periperal_discover_characteristic(&lwp3device->task, &pb_lwp3device_char);
    pb_module_tools_pbio_task_do_blocking(&lwp3device->task, timeout);
}

static mp_obj_t pb_type_pupdevices_Remote_light_on(void *context, const pbio_color_hsv_t *hsv) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    pb_lwp3device_assert_connected();

    static struct {
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
    pbio_set_uint16_le(msg.value.handle, pb_lwp3device_char.handle);

    pbio_color_hsv_to_rgb(hsv, (pbio_color_rgb_t *)msg.payload);

    // The red LED on the handset is weak, so we have to reduce green and blue
    // to get the colors right.
    msg.payload[1] = msg.payload[1] * 3 / 8;
    msg.payload[2] = msg.payload[2] * 3 / 8;

    pbdrv_bluetooth_peripheral_write(&lwp3device->task, &msg.value);
    return pb_module_tools_pbio_task_wait_or_await(&lwp3device->task);
}

static void pb_lwp3device_configure_remote(void) {

    pb_lwp3device_t *remote = &pb_lwp3device_singleton;

    static struct {
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
        .delta_interval = 1,
    };

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {

        pbio_set_uint16_le(msg.value.handle, pb_lwp3device_char.handle);

        // set mode for left buttons

        msg.port = REMOTE_PORT_LEFT_BUTTONS,
        msg.mode = REMOTE_BUTTONS_MODE_KEYSD,
        msg.enable_notifications = 1,
        pbdrv_bluetooth_peripheral_write(&remote->task, &msg.value);
        pb_module_tools_pbio_task_do_blocking(&remote->task, -1);

        // set mode for right buttons

        msg.port = REMOTE_PORT_RIGHT_BUTTONS;
        pbdrv_bluetooth_peripheral_write(&remote->task, &msg.value);
        pb_module_tools_pbio_task_do_blocking(&remote->task, -1);

        // set status light to RGB mode

        msg.port = REMOTE_PORT_STATUS_LIGHT;
        msg.mode = STATUS_LIGHT_MODE_RGB_0;
        msg.enable_notifications = 0;
        pbdrv_bluetooth_peripheral_write(&remote->task, &msg.value);
        pb_module_tools_pbio_task_do_blocking(&remote->task, -1);

        // REVISIT: Could possibly use system color here to make remote match
        // hub status light. For now, the system color is hard-coded to blue.
        pbio_color_hsv_t hsv;
        pbio_color_to_hsv(PBIO_COLOR_BLUE, &hsv);
        pb_type_pupdevices_Remote_light_on(NULL, &hsv);

        nlr_pop();
    } else {
        // disconnect if any setup task failed
        pbdrv_bluetooth_peripheral_disconnect(&remote->task);
        pb_module_tools_pbio_task_do_blocking(&remote->task, -1);
        nlr_jump(nlr.ret_val);
    }
}

void pb_type_lwp3device_start_cleanup(void) {
    static pbio_task_t disconnect_task;
    pbdrv_bluetooth_peripheral_disconnect(&disconnect_task);
    // Task awaited in pybricks de-init.
}

mp_obj_t pb_type_remote_button_pressed(void) {
    pb_lwp3device_t *remote = &pb_lwp3device_singleton;

    pb_lwp3device_assert_connected();

    mp_obj_t pressed[7];
    size_t num = 0;

    if (remote->left[0]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_LEFT_PLUS);
    }
    if (remote->left[1]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_LEFT);
    }
    if (remote->left[2]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_LEFT_MINUS);
    }
    if (remote->right[0]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_RIGHT_PLUS);
    }
    if (remote->right[1]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_RIGHT);
    }
    if (remote->right[2]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_RIGHT_MINUS);
    }
    if (remote->center) {
        pressed[num++] = pb_type_button_new(MP_QSTR_CENTER);
    }

    #if MICROPY_PY_BUILTINS_SET
    return mp_obj_new_set(num, pressed);
    #else
    return mp_obj_new_tuple(num, pressed);
    #endif
}

typedef struct _pb_type_pupdevices_Remote_obj_t {
    mp_obj_base_t base;
    mp_obj_t buttons;
    mp_obj_t light;
} pb_type_pupdevices_Remote_obj_t;

static mp_obj_t pb_type_pupdevices_Remote_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    #if PBSYS_CONFIG_BLUETOOTH_TOGGLE
    if (!pbsys_storage_settings_bluetooth_enabled()) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Bluetooth not enabled"));
    }
    #endif // PBSYS_CONFIG_BLUETOOTH_TOGGLE

    pb_type_pupdevices_Remote_obj_t *self = mp_obj_malloc(pb_type_pupdevices_Remote_obj_t, type);

    const char *name = name_in == mp_const_none ? NULL : mp_obj_str_get_str(name_in);
    mp_int_t timeout = timeout_in == mp_const_none ? -1 : pb_obj_get_positive_int(timeout_in);
    pb_lwp3device_connect(name, timeout, LWP3_HUB_KIND_HANDSET);
    pb_lwp3device_configure_remote();

    self->buttons = pb_type_Keypad_obj_new(pb_type_remote_button_pressed);
    self->light = pb_type_ColorLight_external_obj_new(NULL, pb_type_pupdevices_Remote_light_on);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t pb_lwp3device_name(size_t n_args, const mp_obj_t *args) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    pb_lwp3device_assert_connected();

    if (n_args == 2) {
        size_t len;
        const char *name = mp_obj_str_get_data(args[1], &len);

        if (len == 0 || len > LWP3_MAX_HUB_PROPERTY_NAME_SIZE) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad name length"));
        }

        static struct {
            pbdrv_bluetooth_value_t value;
            uint8_t length;
            uint8_t hub;
            uint8_t type;
            uint8_t property;
            uint8_t operation;
            char payload[LWP3_MAX_HUB_PROPERTY_NAME_SIZE];
        } __attribute__((packed)) msg;

        pbio_set_uint16_le(msg.value.handle, pb_lwp3device_char.handle);
        msg.value.size = msg.length = len + 5;
        msg.hub = 0;
        msg.type = LWP3_MSG_TYPE_HUB_PROPERTIES;
        msg.property = LWP3_HUB_PROPERTY_NAME;
        msg.operation = LWP3_HUB_PROPERTY_OP_SET;
        memcpy(msg.payload, name, len);

        // NB: operation is not cancelable, so timeout is not used
        pbdrv_bluetooth_peripheral_write(&lwp3device->task, &msg.value);
        pb_module_tools_pbio_task_do_blocking(&lwp3device->task, -1);

        // assuming write was successful instead of reading back from the handset
        memcpy(lwp3device->name, name, len);
        lwp3device->name[len] = 0;

        return mp_const_none;
    }

    return mp_obj_new_str(lwp3device->name, strlen(lwp3device->name));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_lwp3device_name_obj, 1, 2, pb_lwp3device_name);

static mp_obj_t pb_lwp3device_disconnect(mp_obj_t self_in) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;
    pb_lwp3device_assert_connected();
    pbdrv_bluetooth_peripheral_disconnect(&lwp3device->task);
    return pb_module_tools_pbio_task_wait_or_await(&lwp3device->task);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_lwp3device_disconnect_obj, pb_lwp3device_disconnect);

static const pb_attr_dict_entry_t pb_type_pupdevices_Remote_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, pb_type_pupdevices_Remote_obj_t, buttons),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, pb_type_pupdevices_Remote_obj_t, light),
    PB_ATTR_DICT_SENTINEL
};

static const mp_rom_map_elem_t pb_type_pupdevices_Remote_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&pb_lwp3device_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_lwp3device_name_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_pupdevices_Remote_locals_dict, pb_type_pupdevices_Remote_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_Remote,
    MP_QSTR_Remote,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_pupdevices_Remote_make_new,
    attr, pb_attribute_handler,
    protocol, pb_type_pupdevices_Remote_attr_dict,
    locals_dict, &pb_type_pupdevices_Remote_locals_dict);

#if PYBRICKS_PY_IODEVICES

static mp_obj_t pb_type_iodevices_LWP3Device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(hub_kind),
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_type_pupdevices_Remote_obj_t *self = mp_obj_malloc(pb_type_pupdevices_Remote_obj_t, type);

    const char *name = name_in == mp_const_none ? NULL : mp_obj_str_get_str(name_in);
    mp_int_t timeout = timeout_in == mp_const_none ? -1 : pb_obj_get_positive_int(timeout_in);
    uint8_t hub_kind = pb_obj_get_positive_int(hub_kind_in);
    pb_lwp3device_connect(name, timeout, hub_kind);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t lwp3device_write(mp_obj_t self_in, mp_obj_t buf_in) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    pb_lwp3device_assert_connected();

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len < LWP3_HEADER_SIZE || bufinfo.len > LWP3_MAX_MESSAGE_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("bad length"));
    }
    if (((uint8_t *)bufinfo.buf)[0] != bufinfo.len) {
        mp_raise_ValueError(MP_ERROR_TEXT("length in header wrong"));
    }

    static struct {
        pbdrv_bluetooth_value_t value;
        char payload[LWP3_MAX_MESSAGE_SIZE];
    } __attribute__((packed)) msg = {
    };
    msg.value.size = bufinfo.len;
    memcpy(msg.payload, bufinfo.buf, bufinfo.len);
    pbio_set_uint16_le(msg.value.handle, pb_lwp3device_char.handle);

    pbdrv_bluetooth_peripheral_write(&lwp3device->task, &msg.value);
    return pb_module_tools_pbio_task_wait_or_await(&lwp3device->task);
}
static MP_DEFINE_CONST_FUN_OBJ_2(lwp3device_write_obj, lwp3device_write);

static mp_obj_t lwp3device_read(mp_obj_t self_in) {
    pb_lwp3device_t *lwp3device = &pb_lwp3device_singleton;

    // wait until a notification is received
    for (;;) {
        pb_lwp3device_assert_connected();

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
static MP_DEFINE_CONST_FUN_OBJ_1(lwp3device_read_obj, lwp3device_read);

static const mp_rom_map_elem_t pb_type_iodevices_LWP3Device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&pb_lwp3device_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_lwp3device_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&lwp3device_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&lwp3device_read_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_iodevices_LWP3Device_locals_dict, pb_type_iodevices_LWP3Device_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_LWP3Device,
    MP_QSTR_LWP3Device,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_iodevices_LWP3Device_make_new,
    locals_dict, &pb_type_iodevices_LWP3Device_locals_dict);

#endif // PYBRICKS_PY_IODEVICES

#endif // PYBRICKS_PY_PUPDEVICES_REMOTE
