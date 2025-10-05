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
#include <pbsys/config.h>
#include <pbsys/status.h>
#include <pbsys/storage_settings.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_async.h>
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
#define LWP3_MAX_MESSAGE_SIZE (23 - LWP3_HEADER_SIZE)

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

// Needed for global notification callback. This is cleared when the finalizer
// runs.
static mp_obj_t self_obj;

typedef struct {
    mp_obj_base_t base;
    pb_type_async_t *iter;
    pbio_os_state_t sub;
    mp_obj_t buttons;
    mp_obj_t light;
    uint8_t left[3];
    uint8_t right[3];
    uint8_t center;
    lwp3_hub_kind_t hub_kind;
    // Null-terminated name used to filter advertisements and responses.
    // Also used as the name of the device when setting the name, since this
    // is not updated in the driver until the next time it connects.
    char name[LWP3_MAX_HUB_PROPERTY_NAME_SIZE + 1];
    #if PYBRICKS_PY_IODEVICES
    /**
     * Maximum number of stored notifications.
     */
    uint8_t noti_num;
    /**
     * Index to read (the oldest data).
     */
    uint8_t noti_idx_read;
    /**
     * Index to write (next free slot).
     */
    uint8_t noti_idx_write;
    /**
     * The buffer is full, next write will override oldest data.
     */
    bool noti_data_full;
    /**
     * Variable length buffer holding multiple LWP3 notifications.
     */
    uint8_t notification_buffer[];
    #endif // PYBRICKS_PY_IODEVICES
} pb_lwp3device_obj_t;

// Handles LEGO Wireless protocol messages from the Powered Up Remote.
static pbio_pybricks_error_t handle_remote_notification(const uint8_t *value, uint32_t size) {

    if (self_obj == MP_OBJ_NULL) {
        // Silently ignore incoming notifications when we aren't expecting any.
        return PBIO_PYBRICKS_ERROR_OK;
    }

    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_obj);

    if (value[0] == 5 && value[2] == LWP3_MSG_TYPE_HW_NET_CMDS && value[3] == LWP3_HW_NET_CMD_CONNECTION_REQ) {
        // This message is meant for something else, but contains the center button state
        self->center = value[4];
    } else if (value[0] == 7 && value[2] == LWP3_MSG_TYPE_PORT_VALUE) {
        // This assumes that the handset button ports have already been set to mode KEYSD
        if (value[3] == REMOTE_PORT_LEFT_BUTTONS) {
            memcpy(self->left, &value[4], 3);
        } else if (value[3] == REMOTE_PORT_RIGHT_BUTTONS) {
            memcpy(self->right, &value[4], 3);
        }
    }

    return PBIO_PYBRICKS_ERROR_OK;
}

static pbdrv_bluetooth_ad_match_result_flags_t lwp3_advertisement_matches(uint8_t event_type, const uint8_t *data, const char *name, const uint8_t *addr, const uint8_t *match_addr) {

    pbdrv_bluetooth_ad_match_result_flags_t flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;

    if (self_obj == MP_OBJ_NULL) {
        return flags;
    }

    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_obj);

    // Whether this looks like a LWP3 advertisement of the correct hub kind.
    if (event_type == PBDRV_BLUETOOTH_AD_TYPE_ADV_IND
        && data[3] == 17 /* length */
        && (data[4] == PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_COMPLETE_LIST
            || data[4] == PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_INCOMPLETE_LIST)
        && pbio_uuid128_reverse_compare(&data[5], pbio_lwp3_hub_service_uuid)
        && data[26] == self->hub_kind) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_VALUE;
    }

    // Compare address in advertisement to previously scanned address.
    if (memcmp(addr, match_addr, 6) == 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_ADDRESS;
    }
    return flags;
}

static pbdrv_bluetooth_ad_match_result_flags_t lwp3_advertisement_response_matches(uint8_t event_type, const uint8_t *data, const char *name, const uint8_t *addr, const uint8_t *match_addr) {

    pbdrv_bluetooth_ad_match_result_flags_t flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;

    if (self_obj == MP_OBJ_NULL) {
        return flags;
    }

    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_obj);

    // This is the only value check we do on LWP3 response messages.
    if (event_type == PBDRV_BLUETOOTH_AD_TYPE_SCAN_RSP) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_VALUE;
    }

    // Compare address in response to previously scanned address.
    if (memcmp(addr, match_addr, 6) == 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_ADDRESS;
    }

    // Compare name to user-provided name if given, checking only up to the
    // user provided name length.
    if (self->name[0] != '\0' && strncmp(name, self->name, strlen(self->name)) != 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_NAME_FAILED;
    }

    return flags;
}

static void pb_lwp3device_assert_connected(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        mp_raise_OSError(MP_ENODEV);
    }
}

static pbdrv_bluetooth_peripheral_connect_config_t scan_config = {
    .match_adv = lwp3_advertisement_matches,
    .match_adv_rsp = lwp3_advertisement_response_matches,
    // other options are variable.
};

static pbio_error_t pb_type_pupdevices_Remote_write_light_msg(mp_obj_t self_in, const pbio_color_hsv_t *hsv) {

    struct {
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
        .length = 10,
        .type = LWP3_MSG_TYPE_OUT_PORT_CMD,
        .port = REMOTE_PORT_STATUS_LIGHT,
        .startup = LWP3_STARTUP_BUFFER,
        .completion = LWP3_COMPLETION_NO_ACTION,
        .cmd = LWP3_OUTPUT_CMD_WRITE_DIRECT_MODE_DATA,
        .mode = STATUS_LIGHT_MODE_RGB_0,
    };
    pbio_color_hsv_to_rgb(hsv, (pbio_color_rgb_t *)msg.payload);

    // The red LED on the handset is weak, so we have to reduce green and blue
    // to get the colors right.
    msg.payload[1] = msg.payload[1] * 3 / 8;
    msg.payload[2] = msg.payload[2] * 3 / 8;

    return pbdrv_bluetooth_peripheral_write_characteristic(pb_lwp3device_char.handle, (const uint8_t *)&msg, sizeof(msg));
}

static mp_obj_t wait_or_await_operation(mp_obj_t self_in) {
    pb_type_async_t config = {
        .iter_once = pbdrv_bluetooth_await_peripheral_command,
        .parent_obj = self_in,
    };
    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return pb_type_async_wait_or_await(&config, &self->iter, true);
}

static mp_obj_t pb_type_pupdevices_Remote_light_on(mp_obj_t self_in, const pbio_color_hsv_t *hsv) {
    pb_assert(pb_type_pupdevices_Remote_write_light_msg(self_in, hsv));
    return wait_or_await_operation(self_in);
}

static pbio_error_t pb_lwp3device_connect_thread(pbio_os_state_t *state, mp_obj_t parent_obj) {

    pbio_os_state_t unused;

    pbio_error_t err;

    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(parent_obj);

    PBIO_OS_ASYNC_BEGIN(state);

    // Scan and connect with timeout.
    err = pbdrv_bluetooth_peripheral_scan_and_connect(&scan_config);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Copy the name so we can read it back later, and override locally.
    memcpy(self->name, pbdrv_bluetooth_peripheral_get_name(), sizeof(self->name));

    // Discover common lwp3 characteristic.
    pb_assert(pbdrv_bluetooth_peripheral_discover_characteristic(&pb_lwp3device_char));
    PBIO_OS_AWAIT(state, &unused, pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    if (!mp_obj_is_type(parent_obj, &pb_type_pupdevices_Remote)) {
        // For generic LWP3 devices there is nothing left to do.
        return PBIO_SUCCESS;
    }

    static struct {
        uint8_t length;
        uint8_t hub;
        uint8_t type;
        uint8_t port;
        uint8_t mode;
        uint32_t delta_interval;
        uint8_t enable_notifications;
    } __attribute__((packed)) msg = {
        .length = 10,
        .hub = 0,
        .type = LWP3_MSG_TYPE_PORT_MODE_SETUP,
        .delta_interval = 1,
    };

    // set mode for left buttons
    msg.port = REMOTE_PORT_LEFT_BUTTONS,
    msg.mode = REMOTE_BUTTONS_MODE_KEYSD,
    msg.enable_notifications = 1,

    err = pbdrv_bluetooth_peripheral_write_characteristic(pb_lwp3device_char.handle, (const uint8_t *)&msg, sizeof(msg));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    // set mode for right buttons
    msg.port = REMOTE_PORT_RIGHT_BUTTONS;

    err = pbdrv_bluetooth_peripheral_write_characteristic(pb_lwp3device_char.handle, (const uint8_t *)&msg, sizeof(msg));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    // set status light to RGB mode
    msg.port = REMOTE_PORT_STATUS_LIGHT;
    msg.mode = STATUS_LIGHT_MODE_RGB_0;
    msg.enable_notifications = 0;

    err = pbdrv_bluetooth_peripheral_write_characteristic(pb_lwp3device_char.handle, (const uint8_t *)&msg, sizeof(msg));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    // set status light to blue.
    pbio_color_hsv_t hsv;
    pbio_color_to_hsv(PBIO_COLOR_BLUE, &hsv);
    err = pb_type_pupdevices_Remote_write_light_msg(parent_obj, &hsv);
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    return PBIO_SUCCESS;

disconnect:
    PBIO_OS_AWAIT(state, &unused, pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    pbdrv_bluetooth_peripheral_disconnect();
    PBIO_OS_AWAIT(state, &unused, pbdrv_bluetooth_await_peripheral_command(&unused, NULL));
    PBIO_OS_ASYNC_END(PBIO_ERROR_IO);
}

static mp_obj_t pb_lwp3device_connect(mp_obj_t self_in, mp_obj_t name_in, mp_obj_t timeout_in, lwp3_hub_kind_t hub_kind, pbdrv_bluetooth_receive_handler_t notification_handler, bool pair) {

    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    self->iter = NULL;

    // needed to ensure that no buttons are "pressed" after reconnecting since
    // we are using static memory
    memset(&self->left, 0, sizeof(self->left));
    memset(&self->right, 0, sizeof(self->right));
    self->center = 0;

    // Hub kind and name are set to filter advertisements and responses.
    self->hub_kind = hub_kind;
    if (name_in == mp_const_none) {
        self->name[0] = '\0';
    } else {
        // Guaranteed to be zero-terminated when using this getter.
        const char *name = mp_obj_str_get_str(name_in);
        size_t len = strlen(name);
        if (len > sizeof(self->name) - 1) {
            mp_raise_ValueError(MP_ERROR_TEXT("Name too long"));
        }
        strncpy(self->name, name, sizeof(self->name));
    }

    scan_config.notification_handler = notification_handler;
    scan_config.options = PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_NONE;
    if (pair) {
        scan_config.options |= PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR;
    }
    scan_config.timeout = timeout_in == mp_const_none ? 0 : pb_obj_get_positive_int(timeout_in) + 1;

    pb_type_async_t config = {
        .iter_once = pb_lwp3device_connect_thread,
        .parent_obj = self_in,
    };
    return pb_type_async_wait_or_await(&config, &self->iter, true);
}


mp_obj_t pb_type_remote_button_pressed(mp_obj_t self_in) {
    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_lwp3device_assert_connected();

    mp_obj_t pressed[7];
    size_t num = 0;

    if (self->left[0]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_LEFT_PLUS);
    }
    if (self->left[1]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_LEFT);
    }
    if (self->left[2]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_LEFT_MINUS);
    }
    if (self->right[0]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_RIGHT_PLUS);
    }
    if (self->right[1]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_RIGHT);
    }
    if (self->right[2]) {
        pressed[num++] = pb_type_button_new(MP_QSTR_RIGHT_MINUS);
    }
    if (self->center) {
        pressed[num++] = pb_type_button_new(MP_QSTR_CENTER);
    }

    #if MICROPY_PY_BUILTINS_SET
    return mp_obj_new_set(num, pressed);
    #else
    return mp_obj_new_tuple(num, pressed);
    #endif
}

mp_obj_t pb_lwp3device_close(mp_obj_t self_in) {
    self_obj = MP_OBJ_NULL;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_lwp3device_close_obj, pb_lwp3device_close);

static mp_obj_t pb_type_pupdevices_Remote_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_module_tools_assert_blocking();

    if (self_obj) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Can use only one Remote"));
    }

    pb_lwp3device_obj_t *self = mp_obj_malloc_with_finaliser(pb_lwp3device_obj_t, type);
    self_obj = MP_OBJ_FROM_PTR(self);

    self->buttons = pb_type_Keypad_obj_new(MP_OBJ_FROM_PTR(self), pb_type_remote_button_pressed);
    self->light = pb_type_ColorLight_external_obj_new(MP_OBJ_FROM_PTR(self), pb_type_pupdevices_Remote_light_on);
    #if PYBRICKS_PY_IODEVICES
    self->noti_num = 0;
    #endif

    pb_lwp3device_connect(MP_OBJ_FROM_PTR(self), name_in, timeout_in, LWP3_HUB_KIND_HANDSET, handle_remote_notification, false);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t pb_lwp3device_name(size_t n_args, const mp_obj_t *args) {

    mp_obj_t self_in = args[0];
    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (n_args == 2) {
        size_t len;
        const char *name = mp_obj_str_get_data(args[1], &len);

        if (len == 0 || len > LWP3_MAX_HUB_PROPERTY_NAME_SIZE) {
            mp_raise_ValueError(MP_ERROR_TEXT("bad name length"));
        }

        struct {
            uint8_t length;
            uint8_t hub;
            uint8_t type;
            uint8_t property;
            uint8_t operation;
            char payload[LWP3_MAX_HUB_PROPERTY_NAME_SIZE];
        } __attribute__((packed)) msg;

        msg.hub = 0;
        msg.type = LWP3_MSG_TYPE_HUB_PROPERTIES;
        msg.property = LWP3_HUB_PROPERTY_NAME;
        msg.operation = LWP3_HUB_PROPERTY_OP_SET;
        memcpy(msg.payload, name, len);

        // assuming write was successful instead of reading back from the handset
        memcpy(self->name, name, len);
        self->name[len] = 0;

        pb_assert(pbdrv_bluetooth_peripheral_write_characteristic(pb_lwp3device_char.handle, (const uint8_t *)&msg, sizeof(msg) - sizeof(msg.payload) + len));
        return wait_or_await_operation(self_in);
    }

    pb_lwp3device_assert_connected();
    return mp_obj_new_str(self->name, strlen(self->name));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_lwp3device_name_obj, 1, 2, pb_lwp3device_name);

static mp_obj_t pb_lwp3device_disconnect(mp_obj_t self_in) {
    pb_assert(pbdrv_bluetooth_peripheral_disconnect());
    return wait_or_await_operation(self_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_lwp3device_disconnect_obj, pb_lwp3device_disconnect);

static const pb_attr_dict_entry_t pb_type_pupdevices_Remote_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, pb_lwp3device_obj_t, buttons),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, pb_lwp3device_obj_t, light),
    PB_ATTR_DICT_SENTINEL
};

static const mp_rom_map_elem_t pb_type_pupdevices_Remote_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_lwp3device_close_obj) },
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

static pbio_pybricks_error_t handle_lwp3_generic_notification(const uint8_t *value, uint32_t size) {

    if (self_obj == MP_OBJ_NULL) {
        // Silently ignore incoming notifications when we aren't expecting any.
        return PBIO_PYBRICKS_ERROR_OK;
    }

    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_obj);

    if (!self->noti_num) {
        // Allocated data not ready, but no error.
        return PBIO_PYBRICKS_ERROR_OK;
    }

    // Buffer is full, so drop oldest sample by advancing read index.
    if (self->noti_data_full) {
        self->noti_idx_read = (self->noti_idx_read + 1) % self->noti_num;
    }

    memcpy(&self->notification_buffer[self->noti_idx_write * LWP3_MAX_MESSAGE_SIZE], &value[0], (size < LWP3_MAX_MESSAGE_SIZE) ? size : LWP3_MAX_MESSAGE_SIZE);
    self->noti_idx_write = (self->noti_idx_write + 1) % self->noti_num;

    // After writing it is full if the _next_ write will override the
    // to-be-read data. If it was already full when we started writing, both
    // indexes have now advanced so it is still full now.
    self->noti_data_full = self->noti_idx_read == self->noti_idx_write;
    return PBIO_PYBRICKS_ERROR_OK;
}


static mp_obj_t pb_type_iodevices_LWP3Device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(hub_kind),
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000),
        PB_ARG_DEFAULT_FALSE(pair),
        PB_ARG_DEFAULT_INT(num_notifications, 8));

    if (self_obj) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Can use only one LWP3Device"));
    }

    uint8_t hub_kind = pb_obj_get_positive_int(hub_kind_in);
    bool pair = mp_obj_is_true(pair_in);

    size_t noti_num = mp_obj_get_int(num_notifications_in);
    if (!noti_num) {
        noti_num = 1;
    }

    pb_lwp3device_obj_t *self = mp_obj_malloc_var_with_finaliser(pb_lwp3device_obj_t, uint8_t, LWP3_MAX_MESSAGE_SIZE * noti_num, type);
    self_obj = MP_OBJ_FROM_PTR(self);

    memset(self->notification_buffer, 0, LWP3_MAX_MESSAGE_SIZE * noti_num);
    self->noti_num = noti_num;
    self->noti_idx_read = 0;
    self->noti_idx_write = 0;
    self->noti_data_full = false;

    pb_module_tools_assert_blocking();

    pb_lwp3device_connect(MP_OBJ_FROM_PTR(self), name_in, timeout_in, hub_kind, handle_lwp3_generic_notification, pair);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t lwp3device_write(mp_obj_t self_in, mp_obj_t buf_in) {

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len < LWP3_HEADER_SIZE || bufinfo.len > LWP3_MAX_MESSAGE_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("bad length"));
    }
    if (((uint8_t *)bufinfo.buf)[0] != bufinfo.len) {
        mp_raise_ValueError(MP_ERROR_TEXT("length in header wrong"));
    }

    pb_assert(pbdrv_bluetooth_peripheral_write_characteristic(pb_lwp3device_char.handle, (const uint8_t *)bufinfo.buf, bufinfo.len));
    return wait_or_await_operation(self_in);
}
static MP_DEFINE_CONST_FUN_OBJ_2(lwp3device_write_obj, lwp3device_write);

static mp_obj_t lwp3device_read(mp_obj_t self_in) {
    pb_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_lwp3device_assert_connected();

    if (!self->noti_num) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    if (!self->noti_data_full && self->noti_idx_write == self->noti_idx_read) {
        return mp_const_none;
    }

    // Update index before returning, else bad values would not be cleared.
    uint8_t index = self->noti_idx_read;
    self->noti_data_full = false;
    self->noti_idx_read = (self->noti_idx_read + 1) % self->noti_num;

    // First byte is the size.
    uint8_t len = self->notification_buffer[index * LWP3_MAX_MESSAGE_SIZE];
    if (len < LWP3_HEADER_SIZE || len > LWP3_MAX_MESSAGE_SIZE) {
        // This is rare but it can happen sometimes. It is better to just
        // ignore it rather than raise and crash the user application.
        return mp_const_none;
    }

    // Allocation of the return object may drive the runloop and process
    // new incoming messages, so copy data atomically before that happens.
    uint8_t message[LWP3_MAX_MESSAGE_SIZE];
    memcpy(message, &self->notification_buffer[index * LWP3_MAX_MESSAGE_SIZE], len);
    return mp_obj_new_bytes(message, len);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lwp3device_read_obj, lwp3device_read);

static const mp_rom_map_elem_t pb_type_iodevices_LWP3Device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_lwp3device_close_obj) },
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

MP_REGISTER_ROOT_POINTER(uint8_t * notification_buffer);

#endif // PYBRICKS_PY_IODEVICES

#endif // PYBRICKS_PY_PUPDEVICES_REMOTE
