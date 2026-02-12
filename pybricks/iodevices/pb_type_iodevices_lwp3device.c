// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2024 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

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
static const uint8_t lwp3_hub_service_uuid[] = {
    0x00, 0x00, 0x16, 0x23, 0x12, 0x12, 0xEF, 0xDE,
    0x16, 0x23, 0x78, 0x5F, 0xEA, 0xBC, 0xD1, 0x23,
};

/**
 * LEGO Wireless Protocol v3 Hub Characteristic UUID.
 *
 * 00001624-1212-EFDE-1623-785FEABCD123
 */
static const uint8_t lwp3_uuid128[] = {
    0x00, 0x00, 0x16, 0x24, 0x12, 0x12, 0xEF, 0xDE,
    0x16, 0x23, 0x78, 0x5F, 0xEA, 0xBC, 0xD1, 0x23,
};

typedef struct {
    mp_obj_base_t base;
    /**
     * The peripheral instance associated with this MicroPython object.
     */
    pbdrv_bluetooth_peripheral_t *peripheral;
    /**
     * The discovered LWP3 characteristic handle.
     */
    uint16_t lwp3_char_handle;
    /**
     * Async iterable state.
     */
    pb_type_async_t *iter;
    /**
     * Additional optional state that can be used by connection threads.
     */
    pbio_os_state_t sub_state;
    /**
     * Buttons object (property of Remote class).
     */
    mp_obj_t buttons;
    /**
     * Light object (property of Remote class).
     */
    mp_obj_t light;
    /**
     * Powered Up Remote Left button states, populated by notifications.
     */
    uint8_t left[3];
    /**
     * Powered Up Remote Right button states, populated by notifications.
     */
    uint8_t right[3];
    /**
     * Powered Up Remote Center button state, populated by notifications.
     */
    uint8_t center;
    /**
     * Routine to run after establishing a connection (e.g. subscribing to ports).
     */
    pbio_os_process_func_t post_connect_setup_func;
    /**
     * Routine to before (re-)establishing a connection (e.g. clearing stale data.)
     */
    mp_fun_1_t pre_connect_func;
    // Null-terminated name used to filter advertisements and responses.
    // Also used as the name of the device when setting the name, since this
    // is not updated in the driver until the next time it connects.
    char name[LWP3_MAX_HUB_PROPERTY_NAME_SIZE + 1];
    /**
     * Scan and connect configuration.
     */
    pbdrv_bluetooth_peripheral_connect_config_t scan_config;
    /**
     * The hub kind to filter advertisements for.
     */
    lwp3_hub_kind_t hub_kind;
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
} pb_type_lwp3device_obj_t;

static bool pb_type_lwp3device_advertisement_matches(void *user, const uint8_t *data, uint8_t length) {

    pb_type_lwp3device_obj_t *self = user;
    if (!self) {
        return false;
    }

    // Whether this looks like a LWP3 advertisement of the correct hub kind.
    return
        data[3] == 17 /* length */
        && (data[4] == PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_COMPLETE_LIST
            || data[4] == PBDRV_BLUETOOTH_AD_DATA_TYPE_128_BIT_SERV_UUID_INCOMPLETE_LIST)
        && pbio_uuid128_reverse_compare(&data[5], lwp3_hub_service_uuid)
        && data[26] == self->hub_kind;
}

static bool pb_type_lwp3device_advertisement_response_matches(void *user, const uint8_t *data, uint8_t length) {

    pb_type_lwp3device_obj_t *self = user;
    if (!self) {
        return false;
    }

    // Pass if no name filter specified or the given name matches, checking only up to provided name length.
    return self->name[0] == '\0' || strncmp((const char *)&data[2], self->name, strlen(self->name)) == 0;
}

static mp_obj_t wait_or_await_operation(mp_obj_t self_in) {

    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_type_async_t config = {
        .iter_once = pbdrv_bluetooth_await_peripheral_command,
        // Using the driver function without a wrapper, so should pass its
        // context parameter (the peripheral) as the parent object.
        .parent_obj = self->peripheral,
    };
    return pb_type_async_wait_or_await(&config, &self->iter, true);
}

static mp_obj_t pb_type_lwp3device_close(mp_obj_t self_in) {
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Disables notification handler from accessing allocated memory.
    pbdrv_bluetooth_peripheral_release(self->peripheral, self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_lwp3device_close_obj, pb_type_lwp3device_close);

static pbio_error_t pb_type_lwp3device_connect_thread(pbio_os_state_t *state, mp_obj_t parent_obj) {

    pbio_os_state_t unused;

    pbio_error_t err;

    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(parent_obj);

    PBIO_OS_ASYNC_BEGIN(state);

    // Get available peripheral instance.
    pb_assert(pbdrv_bluetooth_peripheral_get_available(&self->peripheral, self));

    // Initiate scan and connect with timeout.
    pb_assert(pbdrv_bluetooth_peripheral_scan_and_connect(self->peripheral, &self->scan_config));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    if (err != PBIO_SUCCESS) {
        // Not successful, release peripheral.
        pb_type_lwp3device_close(parent_obj);
        return err;
    }

    // Copy the name so we can read it back later, and override locally.
    memcpy(self->name, pbdrv_bluetooth_peripheral_get_name(self->peripheral), sizeof(self->name));

    // Discover common lwp3 characteristic.
    pbdrv_bluetooth_peripheral_char_discovery_t pb_type_lwp3device_char = {
        .properties = 0x001c, // Notify, Write Without Response, Write
        .uuid128 = lwp3_uuid128,
        .request_notification = true,
    };
    pb_assert(pbdrv_bluetooth_peripheral_discover_characteristic(self->peripheral, &pb_type_lwp3device_char));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    self->lwp3_char_handle = pbdrv_bluetooth_peripheral_discover_characteristic_get_result(self->peripheral);
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    // Post connection commands such as configuring port subscriptions.
    if (self->post_connect_setup_func) {
        PBIO_OS_AWAIT(state, &self->sub_state, err = self->post_connect_setup_func(&self->sub_state, parent_obj));
        if (err != PBIO_SUCCESS) {
            goto disconnect;
        }
    }

    return PBIO_SUCCESS;

disconnect:
    pb_type_lwp3device_close(parent_obj);
    pb_assert(pbdrv_bluetooth_peripheral_disconnect(self->peripheral));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    pb_assert(err);
    PBIO_OS_ASYNC_END(PBIO_ERROR_IO);
}

/**
 * Sets the name filter and timeout to use for connect and reconnect.
 */
static void pb_type_lwp3device_set_name_filter_and_timeout(pb_type_lwp3device_obj_t *self, mp_obj_t name_in, mp_obj_t timeout_in) {
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

    // Internally uses 0 for indefinite or positive value for finite.
    self->scan_config.timeout = timeout_in == mp_const_none ? 0 : pb_obj_get_positive_int(timeout_in) + 1;
}

static mp_obj_t pb_type_lwp3device_connect(mp_obj_t self_in) {
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_async_t config = {
        .iter_once = pb_type_lwp3device_connect_thread,
        .parent_obj = self_in,
    };
    return pb_type_async_wait_or_await(&config, &self->iter, true);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_lwp3device_connect_obj, pb_type_lwp3device_connect);

static mp_obj_t pb_type_lwp3device_disconnect(mp_obj_t self_in) {
    // Needed to release claim on allocated data so we can make a new
    // connection later.
    pb_type_lwp3device_close(self_in);
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbdrv_bluetooth_peripheral_disconnect(self->peripheral));
    return wait_or_await_operation(self_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_lwp3device_disconnect_obj, pb_type_lwp3device_disconnect);

static void pb_type_lwp3device_intialize_connection(mp_obj_t self_in, mp_obj_t connect_in) {
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    bool want_connection = mp_obj_is_true(connect_in);

    // Reinitialize or clear data before connecting.
    if (self->pre_connect_func) {
        self->pre_connect_func(self_in);
    }

    // Attempt to re-use existing connection.
    pbio_error_t err = pbdrv_bluetooth_peripheral_get_connected(&self->peripheral, self, &self->scan_config);

    // If we aren't already connected, do so now if requested.
    if (err == PBIO_ERROR_NO_DEV && want_connection) {
        pb_type_lwp3device_connect(self_in);
    }
    // If being connected now is not desired, disconnect.
    else if (err == PBIO_SUCCESS && !want_connection) {
        pb_type_lwp3device_disconnect(self_in);
    }

    // If we have reconnected virtually, we're skipping the discovery phase,
    // so use the result from last time.
    if (pbdrv_bluetooth_peripheral_is_connected(self->peripheral)) {
        self->lwp3_char_handle = pbdrv_bluetooth_peripheral_discover_characteristic_get_result(self->peripheral);
    }
}

static mp_obj_t pb_type_lwp3device_name(size_t n_args, const mp_obj_t *args) {

    mp_obj_t self_in = args[0];
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

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

        pb_assert(pbdrv_bluetooth_peripheral_write_characteristic(
            self->peripheral, self->lwp3_char_handle,
            (const uint8_t *)&msg, sizeof(msg) - sizeof(msg.payload) + len));
        return wait_or_await_operation(self_in);
    }

    if (!pbdrv_bluetooth_peripheral_is_connected(self->peripheral)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    return mp_obj_new_str(self->name, strlen(self->name));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_type_lwp3device_name_obj, 1, 2, pb_type_lwp3device_name);

// -----------------------------------------------------------------------------
// pybricks.pupdevices.Remote (special case of LWP3).
// -----------------------------------------------------------------------------

static void pb_type_remote_handle_notification(void *user, const uint8_t *value, uint32_t size) {
    pb_type_lwp3device_obj_t *self = user;
    if (!self) {
        return;
    }
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
}

mp_obj_t pb_type_remote_button_pressed(mp_obj_t self_in) {
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!pbdrv_bluetooth_peripheral_is_connected(self->peripheral)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

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

static pbio_error_t pb_type_remote_write_light_msg(mp_obj_t self_in, const pbio_color_hsv_t *hsv) {

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

    pbio_color_rgb_t rgb;
    pbio_color_hsv_to_rgb(hsv, &rgb);

    // The red LED on the handset is weak, so we have to reduce green and blue
    // to get the colors right.
    msg.payload[0] = rgb.r;
    msg.payload[1] = rgb.g * 3 / 8;
    msg.payload[2] = rgb.b * 3 / 8;

    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    return pbdrv_bluetooth_peripheral_write_characteristic(self->peripheral, self->lwp3_char_handle, (const uint8_t *)&msg, sizeof(msg));
}

static mp_obj_t pb_type_remote_light_on(mp_obj_t self_in, const pbio_color_hsv_t *hsv) {
    pb_assert(pb_type_remote_write_light_msg(self_in, hsv));
    return wait_or_await_operation(self_in);
}

static pbio_error_t pb_type_remote_post_connect(pbio_os_state_t *state, mp_obj_t parent_obj) {

    pbio_os_state_t unused;

    pbio_error_t err;

    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(parent_obj);

    PBIO_OS_ASYNC_BEGIN(state);

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
    msg.port = REMOTE_PORT_LEFT_BUTTONS;
    msg.mode = REMOTE_BUTTONS_MODE_KEYSD;
    msg.enable_notifications = 1;

    err = pbdrv_bluetooth_peripheral_write_characteristic(self->peripheral, self->lwp3_char_handle, (const uint8_t *)&msg, sizeof(msg));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // set mode for right buttons
    msg.port = REMOTE_PORT_RIGHT_BUTTONS;

    err = pbdrv_bluetooth_peripheral_write_characteristic(self->peripheral, self->lwp3_char_handle, (const uint8_t *)&msg, sizeof(msg));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // set status light to RGB mode
    msg.port = REMOTE_PORT_STATUS_LIGHT;
    msg.mode = STATUS_LIGHT_MODE_RGB_0;
    msg.enable_notifications = 0;

    err = pbdrv_bluetooth_peripheral_write_characteristic(self->peripheral, self->lwp3_char_handle, (const uint8_t *)&msg, sizeof(msg));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // set status light to blue.
    pbio_color_hsv_t hsv;
    pbio_color_to_hsv(PBIO_COLOR_BLUE, &hsv);
    err = pb_type_remote_write_light_msg(parent_obj, &hsv);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    if (err != PBIO_SUCCESS) {
        return err;
    }
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static mp_obj_t pb_type_remote_clear(mp_obj_t self_in) {
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Ensure that no buttons are "pressed" after reconnecting
    memset(&self->left, 0, sizeof(self->left));
    memset(&self->right, 0, sizeof(self->right));
    self->center = 0;
    return mp_const_none;
}

static mp_obj_t pb_type_remote_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000),
        PB_ARG_DEFAULT_TRUE(connect)
        );

    pb_module_tools_assert_blocking();

    pb_type_lwp3device_obj_t *self = mp_obj_malloc_with_finaliser(pb_type_lwp3device_obj_t, type);
    self->iter = NULL;
    self->noti_num = 0;

    self->hub_kind = LWP3_HUB_KIND_HANDSET;

    self->post_connect_setup_func = pb_type_remote_post_connect;
    self->pre_connect_func = pb_type_remote_clear;

    self->scan_config = (pbdrv_bluetooth_peripheral_connect_config_t) {
        .match_adv = pb_type_lwp3device_advertisement_matches,
        .match_adv_rsp = pb_type_lwp3device_advertisement_response_matches,
        .notification_handler = pb_type_remote_handle_notification,
        .options = PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_NONE,
    };
    pb_type_lwp3device_set_name_filter_and_timeout(self, name_in, timeout_in);

    self->buttons = pb_type_Keypad_obj_new(MP_OBJ_FROM_PTR(self), pb_type_remote_button_pressed);
    self->light = pb_type_ColorLight_external_obj_new(MP_OBJ_FROM_PTR(self), pb_type_remote_light_on);

    pb_type_lwp3device_intialize_connection(MP_OBJ_FROM_PTR(self), connect_in);
    return MP_OBJ_FROM_PTR(self);
}

static const pb_attr_dict_entry_t pb_type_remote_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, pb_type_lwp3device_obj_t, buttons),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, pb_type_lwp3device_obj_t, light),
    PB_ATTR_DICT_SENTINEL
};

static const mp_rom_map_elem_t pb_type_remote_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_type_lwp3device_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&pb_type_lwp3device_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&pb_type_lwp3device_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_type_lwp3device_name_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_remote_locals_dict, pb_type_remote_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_remote,
    MP_QSTR_Remote,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_remote_make_new,
    attr, pb_attribute_handler,
    protocol, pb_type_remote_attr_dict,
    locals_dict, &pb_type_remote_locals_dict);

// -----------------------------------------------------------------------------
// pybricks.iodevices.LWP3Device (most generic LWP3).
// -----------------------------------------------------------------------------

static void pb_type_lwp3device_handle_notification_generic(void *user, const uint8_t *value, uint32_t size) {

    pb_type_lwp3device_obj_t *self = user;
    if (!self || !self->noti_num) {
        // Allocated data not ready.
        return;
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
}


static mp_obj_t pb_type_iodevices_lwp3device_clear(mp_obj_t self_in) {
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Clear past notifications.
    memset(self->notification_buffer, 0, LWP3_MAX_MESSAGE_SIZE * self->noti_num);
    self->noti_idx_read = 0;
    self->noti_idx_write = 0;
    self->noti_data_full = false;
    return mp_const_none;
}

static mp_obj_t pb_type_lwp3device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(hub_kind),
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000),
        PB_ARG_DEFAULT_FALSE(pair),
        PB_ARG_DEFAULT_INT(num_notifications, 8),
        PB_ARG_DEFAULT_TRUE(connect)
        );

    size_t noti_num = mp_obj_get_int(num_notifications_in);
    if (!noti_num) {
        noti_num = 1;
    }

    pb_type_lwp3device_obj_t *self = mp_obj_malloc_var_with_finaliser(pb_type_lwp3device_obj_t, uint8_t, LWP3_MAX_MESSAGE_SIZE * noti_num, type);
    self->iter = NULL;
    self->noti_num = noti_num;

    self->hub_kind = mp_obj_get_int(hub_kind_in);
    self->scan_config = (pbdrv_bluetooth_peripheral_connect_config_t) {
        .match_adv = pb_type_lwp3device_advertisement_matches,
        .match_adv_rsp = pb_type_lwp3device_advertisement_response_matches,
        .notification_handler = pb_type_lwp3device_handle_notification_generic,
        .options = mp_obj_is_true(pair_in) ? PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR : PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_NONE,
    };
    self->pre_connect_func = pb_type_iodevices_lwp3device_clear;
    self->post_connect_setup_func = NULL;
    pb_type_lwp3device_set_name_filter_and_timeout(self, name_in, timeout_in);

    pb_module_tools_assert_blocking();

    pb_type_lwp3device_intialize_connection(MP_OBJ_FROM_PTR(self), connect_in);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t lwp3device_write(mp_obj_t self_in, mp_obj_t buf_in) {

    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    if (bufinfo.len < LWP3_HEADER_SIZE || bufinfo.len > LWP3_MAX_MESSAGE_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("bad length"));
    }
    if (((uint8_t *)bufinfo.buf)[0] != bufinfo.len) {
        mp_raise_ValueError(MP_ERROR_TEXT("length in header wrong"));
    }

    pb_assert(pbdrv_bluetooth_peripheral_write_characteristic(self->peripheral, self->lwp3_char_handle, (const uint8_t *)bufinfo.buf, bufinfo.len));
    return wait_or_await_operation(self_in);
}
static MP_DEFINE_CONST_FUN_OBJ_2(lwp3device_write_obj, lwp3device_write);

static mp_obj_t lwp3device_read(mp_obj_t self_in) {
    pb_type_lwp3device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!pbdrv_bluetooth_peripheral_is_connected(self->peripheral)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

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

static const mp_rom_map_elem_t pb_type_lwp3device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_type_lwp3device_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&pb_type_lwp3device_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&pb_type_lwp3device_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_type_lwp3device_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&lwp3device_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&lwp3device_read_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_lwp3device_locals_dict, pb_type_lwp3device_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_lwp3device,
    MP_QSTR_LWP3Device,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_lwp3device_make_new,
    locals_dict, &pb_type_lwp3device_locals_dict);

MP_REGISTER_ROOT_POINTER(uint8_t * notification_buffer);

#endif // PYBRICKS_PY_PUPDEVICES
