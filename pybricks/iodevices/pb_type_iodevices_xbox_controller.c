// Copyright (C) 2024 The Pybricks Authors - All rights reserved
// To build the firmware without this non-free component, set
// PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER to 0 in mpconfigport.h.

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER

#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/debug.h>
#include <pbio/error.h>
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

#define DEBUG 0
#if DEBUG
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

typedef struct __attribute__((packed)) {
    uint16_t x; // left to right
    uint16_t y; // bottom to top
    uint16_t z; // left to right
    uint16_t rz; // bottom to top
    uint16_t left_trigger; // 10-bit
    uint16_t right_trigger; // 10-bit
    uint8_t dpad;
    uint16_t buttons;
    uint8_t upload;
    // Following only available on Elite Series 2 controller.
    uint8_t profile;
    uint8_t trigger_switches;
    uint8_t paddles;
} xbox_input_map_t;

typedef struct _pb_type_xbox_obj_t {
    mp_obj_base_t base;
    /**
     * The peripheral instance associated with this MicroPython object.
     */
    pbdrv_bluetooth_peripheral_t *peripheral;
    /**
     * Null-terminated name used to filter advertisements and responses.
     */
    char name[PBDRV_BLUETOOTH_MAX_ADV_SIZE - 2];
    /**
     * The timeout used during scan and connect.
     */
    uint32_t scan_timeout;
    /**
     * Whether to disconnect from host before connecting to controller.
     **/
    bool disconnect_host;
    /**
     * Timer used to delay between connection attempts.
     */
    pbio_os_timer_t retry_timer;
    /**
     * Buttons object on the Xbox Controller instance.
     */
    mp_obj_t buttons;
    /**
     * Threshold below which joystick reports zero to avoid drift.
     */
    mp_int_t joystick_deadzone;
    /**
     * State of awaitable used for connecting and writing.
     */
    pb_type_async_t *iter;
    /**
     * Button and joystick state (populated by notification handler).
     */
    xbox_input_map_t input_map;
    /**
     * Discovered HID Map characteristic handle.
     */
    uint16_t hid_map_char_handle;
    /**
     * Discovered HID Report characteristic handle.
     */
    uint16_t hid_report_char_handle;
} pb_type_xbox_obj_t;

// Handles LEGO Wireless protocol messages from the XBOX Device.
static void pb_type_xbox_handle_notification(void *user, const uint8_t *value, uint32_t size) {

    pb_type_xbox_obj_t *self = user;
    if (!self) {
        return;
    }

    if (size <= sizeof(xbox_input_map_t)) {
        memcpy(&self->input_map, &value[0], size);
    }
}

#define _16BIT_AS_LE(x) ((x) & 0xff), (((x) >> 8) & 0xff)

static bool pb_type_xbox_advertisement_matches(void *user, const uint8_t *data, uint8_t length) {

    // The controller seems to advertise three different packets, so allow all.

    const uint8_t advertising_data1[] = {
        // Type field for BLE-enabled.
        0x02, PBDRV_BLUETOOTH_AD_DATA_TYPE_FLAGS, 0x06,
        // Type field for appearance (HID Gamepad)
        0x03, PBDRV_BLUETOOTH_AD_DATA_TYPE_APPEARANCE, _16BIT_AS_LE(964),
        // Type field for manufacturer data (Microsoft).
        0x06, PBDRV_BLUETOOTH_AD_DATA_TYPE_MANUFACTURER_DATA, 0x06, 0x00, 0x03, 0x00, 0x80,
    };

    const uint8_t advertising_data2[] = {
        // Type field for BLE-enabled.
        0x02, PBDRV_BLUETOOTH_AD_DATA_TYPE_FLAGS, 0x06,
        // Type for TX power level, could be used to estimate distance.
        0x02, PBDRV_BLUETOOTH_AD_DATA_TYPE_TX_POWER_LEVEL, 0x14,
        // Type field for appearance (HID Gamepad)
        0x03, PBDRV_BLUETOOTH_AD_DATA_TYPE_APPEARANCE, _16BIT_AS_LE(964),
        // Type field for manufacturer data (Microsoft).
        0x04, PBDRV_BLUETOOTH_AD_DATA_TYPE_MANUFACTURER_DATA, 06, 00, 00,
        // List of UUIDs (HID BLE Service)
        0x03, PBDRV_BLUETOOTH_AD_DATA_TYPE_16_BIT_SERV_UUID_COMPLETE_LIST, _16BIT_AS_LE(0x1812),
    };

    // As above, but without discovery mode. This is advertised if the
    // controller is turned on but not in pairing mode. We can only connect
    // to the controller in this case if the hub is the most recent connection
    // to that controller.
    uint8_t advertising_data3[sizeof(advertising_data2)];
    memcpy(advertising_data3, advertising_data2, sizeof(advertising_data2));
    advertising_data3[2] = 0x04;

    // At least one must match.
    return
        !memcmp(data, advertising_data1, sizeof(advertising_data1)) ||
        !memcmp(data, advertising_data2, sizeof(advertising_data2)) ||
        !memcmp(data, advertising_data3, sizeof(advertising_data3));
}

static bool pb_type_xbox_advertisement_response_matches(void *user, const uint8_t *data, uint8_t length) {
    pb_type_xbox_obj_t *self = user;
    if (!self) {
        return false;
    }
    // Pass if no name filter specified or the given name matches, checking only up to provided name length.
    return self->name[0] == '\0' || strncmp((const char *)&data[2], self->name, strlen(self->name)) == 0;
}

static xbox_input_map_t *pb_type_xbox_get_input(mp_obj_t self_in) {

    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!pbdrv_bluetooth_peripheral_is_connected(self->peripheral)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    return &self->input_map;
}

static mp_obj_t pb_type_xbox_button_pressed(mp_obj_t self_in) {

    xbox_input_map_t *buttons = pb_type_xbox_get_input(self_in);

    // At most 16 simultaneous button presses, plus up to two dpad directions.
    mp_obj_t items[16 + 2];
    size_t count = 0;

    if (buttons->buttons & 1) {
        items[count++] = pb_type_button_new(MP_QSTR_A);
    }
    if (buttons->buttons & (1 << 1)) {
        items[count++] = pb_type_button_new(MP_QSTR_B);
    }
    if (buttons->buttons & (1 << 3)) {
        items[count++] = pb_type_button_new(MP_QSTR_X);
    }
    if (buttons->buttons & (1 << 4)) {
        items[count++] = pb_type_button_new(MP_QSTR_Y);
    }
    if (buttons->buttons & (1 << 6)) {
        items[count++] = pb_type_button_new(MP_QSTR_LB);
    }
    if (buttons->buttons & (1 << 7)) {
        items[count++] = pb_type_button_new(MP_QSTR_RB);
    }
    if (buttons->buttons & (1 << 10)) {
        items[count++] = pb_type_button_new(MP_QSTR_VIEW);
    }
    if (buttons->buttons & (1 << 11)) {
        items[count++] = pb_type_button_new(MP_QSTR_MENU);
    }
    if (buttons->buttons & (1 << 12)) {
        items[count++] = pb_type_button_new(MP_QSTR_GUIDE);
    }
    if (buttons->buttons & (1 << 13)) {
        items[count++] = pb_type_button_new(MP_QSTR_LJ);
    }
    if (buttons->buttons & (1 << 14)) {
        items[count++] = pb_type_button_new(MP_QSTR_RJ);
    }
    if (buttons->upload) {
        items[count++] = pb_type_button_new(MP_QSTR_UPLOAD);
    }
    if (buttons->paddles & (1 << 0)) {
        items[count++] = pb_type_button_new(MP_QSTR_P1);
    }
    if (buttons->paddles & (1 << 1)) {
        items[count++] = pb_type_button_new(MP_QSTR_P2);
    }
    if (buttons->paddles & (1 << 2)) {
        items[count++] = pb_type_button_new(MP_QSTR_P3);
    }
    if (buttons->paddles & (1 << 3)) {
        items[count++] = pb_type_button_new(MP_QSTR_P4);
    }

    // Dpad is available as separate method, but can also be used as
    // a normal set of buttons.
    if (buttons->dpad == 1) {
        items[count++] = pb_type_button_new(MP_QSTR_UP);
    } else if (buttons->dpad == 2) {
        items[count++] = pb_type_button_new(MP_QSTR_UP);
        items[count++] = pb_type_button_new(MP_QSTR_RIGHT);
    } else if (buttons->dpad == 3) {
        items[count++] = pb_type_button_new(MP_QSTR_RIGHT);
    } else if (buttons->dpad == 4) {
        items[count++] = pb_type_button_new(MP_QSTR_RIGHT);
        items[count++] = pb_type_button_new(MP_QSTR_DOWN);
    } else if (buttons->dpad == 5) {
        items[count++] = pb_type_button_new(MP_QSTR_DOWN);
    } else if (buttons->dpad == 6) {
        items[count++] = pb_type_button_new(MP_QSTR_DOWN);
        items[count++] = pb_type_button_new(MP_QSTR_LEFT);
    } else if (buttons->dpad == 7) {
        items[count++] = pb_type_button_new(MP_QSTR_LEFT);
    } else if (buttons->dpad == 8) {
        items[count++] = pb_type_button_new(MP_QSTR_LEFT);
        items[count++] = pb_type_button_new(MP_QSTR_UP);
    }

    return mp_obj_new_set(count, items);
}

static mp_obj_t pb_type_xbox_close(mp_obj_t self_in) {
    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Disables notification handler from accessing allocated memory.
    pbdrv_bluetooth_peripheral_release(self->peripheral, self);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_close_obj, pb_type_xbox_close);

static pbio_error_t xbox_connect_thread(pbio_os_state_t *state, mp_obj_t parent_obj) {

    pbio_os_state_t unused;

    pbio_error_t err;

    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(parent_obj);

    PBIO_OS_ASYNC_BEGIN(state);

    // Get available peripheral instance.
    pb_assert(pbdrv_bluetooth_peripheral_get_available(&self->peripheral, self));
    pbio_os_timer_set(&self->retry_timer, 0);

    // Connect with bonding enabled. On Technic Hub, the driver will take care
    // of disconnecting from Pybricks Code if needed.
retry:
    DEBUG_PRINT("Attempt to find XBOX controller and connect and pair.\n");
    pbdrv_bluetooth_peripheral_connect_config_t scan_config = {
        .match_adv = pb_type_xbox_advertisement_matches,
        .match_adv_rsp = pb_type_xbox_advertisement_response_matches,
        .notification_handler = pb_type_xbox_handle_notification,
        .options = PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_PAIR,
        .timeout = self->scan_timeout,
    };
    if (self->disconnect_host) {
        scan_config.options |= PBDRV_BLUETOOTH_PERIPHERAL_OPTIONS_DISCONNECT_HOST;
    }

    pb_assert(pbdrv_bluetooth_peripheral_scan_and_connect(self->peripheral, &scan_config));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));

    // On Prime Hub, connecting can fail for a variety of reasons. A second
    // attempt usually succeeds.
    if (err != PBIO_SUCCESS && !self->retry_timer.duration) {
        DEBUG_PRINT("XBOX controller failed to connect with error %d.\n", err);
        PBIO_OS_AWAIT_MS(state, &self->retry_timer, 2000);
        goto retry;
    }

    if (err != PBIO_SUCCESS) {
        // Not successful, release peripheral.
        pb_type_xbox_close(parent_obj);
    }

    pb_assert(err);
    DEBUG_PRINT("Connected to XBOX controller. Discovering HID map.\n");
    // It seems we need to read the (unused) map only once after pairing
    // to make the controller active. We'll still read it every time to
    // catch the case where user might not have done this at least once.
    // Connecting takes about a second longer this way, but we can provide
    // better error messages.
    pbdrv_bluetooth_peripheral_char_discovery_t char_hid_map = {
        .uuid16 = 0x2a4b,
        .request_notification = false,
    };
    pb_assert(pbdrv_bluetooth_peripheral_discover_characteristic(self->peripheral, &char_hid_map));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    self->hid_map_char_handle = pbdrv_bluetooth_peripheral_discover_characteristic_get_result(self->peripheral);
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    DEBUG_PRINT("Read HID map.\n");
    pb_assert(pbdrv_bluetooth_peripheral_read_characteristic(self->peripheral, self->hid_map_char_handle));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    // This is the main characteristic that notifies us of button state.
    DEBUG_PRINT("Discover HID report.\n");
    pbdrv_bluetooth_peripheral_char_discovery_t pb_type_xbox_char_hid_report = {
        // Even with the property filter, there are still 3 matches for this
        // characteristic on the Elite Series 2 controller. For now limit discovery
        // to find only the first one. It may be possible to find the right one by
        // reading the descriptor instead.
        .handle_max = 32,
        .properties = 0x12, // Needed to distinguish it from another char with same UUID.
        .uuid16 = 0x2a4d,
        .request_notification = true,
    };
    pb_assert(pbdrv_bluetooth_peripheral_discover_characteristic(self->peripheral, &pb_type_xbox_char_hid_report));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    self->hid_report_char_handle = pbdrv_bluetooth_peripheral_discover_characteristic_get_result(self->peripheral);
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    DEBUG_PRINT("Read HID report.\n");
    pb_assert(pbdrv_bluetooth_peripheral_read_characteristic(self->peripheral, self->hid_report_char_handle));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    if (err != PBIO_SUCCESS) {
        goto disconnect;
    }

    return PBIO_SUCCESS;

disconnect:
    pb_type_xbox_close(parent_obj);
    DEBUG_PRINT("Going to disconnect because of a failure with code %d at line %u.\n", err, *state);
    pb_assert(pbdrv_bluetooth_peripheral_disconnect(self->peripheral));
    PBIO_OS_AWAIT(state, &unused, err = pbdrv_bluetooth_await_peripheral_command(&unused, self->peripheral));
    DEBUG_PRINT("Disconnect with error code %d.\n", err);
    pb_assert(err);

    // If the controller was most recently connected to another device like the
    // actual Xbox or a phone, the controller needs to be not just turned on,
    // but also put into pairing mode before connecting to the hub. Otherwise,
    // it will appear to connect and even bond, but return errors when trying
    // to read the HID characteristics. So inform the user to press/hold the
    // pair button to put it into the right mode.

    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT(
        "Connected, but not allowed to read buttons. "
        "Try putting the controller in pairing mode."
        ));

    PBIO_OS_ASYNC_END(PBIO_ERROR_IO);
}

static mp_obj_t pb_type_xbox_connect(mp_obj_t self_in) {
    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_async_t config = {
        .iter_once = xbox_connect_thread,
        .parent_obj = MP_OBJ_FROM_PTR(self),
    };
    return pb_type_async_wait_or_await(&config, &self->iter, true);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_connect_obj, pb_type_xbox_connect);

static mp_obj_t pb_type_xbox_await_operation(mp_obj_t self_in) {
    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_async_t config = {
        .iter_once = pbdrv_bluetooth_await_peripheral_command,
        // Using the driver function without a wrapper, so should pass its
        // context parameter (the peripheral) as the parent object.
        .parent_obj = self->peripheral,
    };
    return pb_type_async_wait_or_await(&config, &self->iter, true);
}

static mp_obj_t pb_type_xbox_disconnect(mp_obj_t self_in) {
    // Needed to release claim on allocated data so we can make a new
    // connection later.
    pb_type_xbox_close(self_in);
    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbdrv_bluetooth_peripheral_disconnect(self->peripheral));
    return pb_type_xbox_await_operation(self_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_disconnect_obj, pb_type_xbox_disconnect);

static mp_obj_t pb_type_xbox_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_INT(joystick_deadzone, 10),
        PB_ARG_DEFAULT_NONE(name),
        PB_ARG_DEFAULT_INT(timeout, 10000),
        PB_ARG_DEFAULT_TRUE(connect)
        // Debug parameter to stay connected to the host on Technic Hub.
        // Works only on some hosts for the moment, so False by default.
        #if PYBRICKS_HUB_TECHNICHUB
        , PB_ARG_DEFAULT_FALSE(stay_connected)
        #endif // PYBRICKS_HUB_TECHNICHUB
        );

    pb_module_tools_assert_blocking();

    pb_type_xbox_obj_t *self = mp_obj_malloc_with_finaliser(pb_type_xbox_obj_t, type);
    self->joystick_deadzone = pb_obj_get_pct(joystick_deadzone_in);
    self->iter = NULL;

    self->buttons = pb_type_Keypad_obj_new(MP_OBJ_FROM_PTR(self), pb_type_xbox_button_pressed);

    // needed to ensure that no buttons are "pressed" since we are using
    // allocated memory
    xbox_input_map_t *input = &self->input_map;
    memset(input, 0, sizeof(xbox_input_map_t));
    input->x = input->y = input->z = input->rz = INT16_MAX;

    self->scan_timeout = timeout_in == mp_const_none ? 0 : pb_obj_get_positive_int(timeout_in) + 1;
    if (name_in == mp_const_none) {
        self->name[0] = '\0';
    } else {
        const char *name = mp_obj_str_get_str(name_in);
        size_t len = strlen(name);
        if (len > sizeof(self->name) - 1) {
            mp_raise_ValueError(MP_ERROR_TEXT("Name too long"));
        }
        strncpy(self->name, name, sizeof(self->name));
    }

    // By default, disconnect Technic Hub from host, as this is required for
    // most hosts. Stay connected only if the user explicitly requests it.
    #if PYBRICKS_HUB_TECHNICHUB
    self->disconnect_host = !mp_obj_is_true(stay_connected_in);
    if (self->disconnect_host) {
        mp_printf(&mp_plat_print, "The hub may disconnect from the computer for better connectivity with the controller.\n");
        mp_hal_delay_ms(500);
    }
    #endif // PYBRICKS_HUB_TECHNICHUB

    bool want_connection = mp_obj_is_true(connect_in);

    // Attempt to re-use existing connection.
    pbdrv_bluetooth_peripheral_connect_config_t scan_config = {
        .match_adv = pb_type_xbox_advertisement_matches,
        .match_adv_rsp = pb_type_xbox_advertisement_response_matches,
        .notification_handler = pb_type_xbox_handle_notification,
    };
    pbio_error_t err = pbdrv_bluetooth_peripheral_get_connected(&self->peripheral, self, &scan_config);

    // If we aren't already connected, do so now if requested.
    if (err == PBIO_ERROR_NO_DEV && want_connection) {
        pb_type_xbox_connect(MP_OBJ_FROM_PTR(self));
    }
    // If being connected now is not desired, disconnect.
    else if (err == PBIO_SUCCESS && !want_connection) {
        pb_type_xbox_disconnect(MP_OBJ_FROM_PTR(self));
    }
    // Other combinations are already in the desired state, so do nothing else.

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t pb_type_xbox_name(size_t n_args, const mp_obj_t *args) {
    // Asserts connection.
    pb_type_xbox_get_input(args[0]);
    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *name = pbdrv_bluetooth_peripheral_get_name(self->peripheral);
    return mp_obj_new_str(name, strlen(name));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_type_xbox_name_obj, 1, 2, pb_type_xbox_name);

static mp_obj_t pb_type_xbox_state(mp_obj_t self_in) {

    xbox_input_map_t *buttons = pb_type_xbox_get_input(self_in);

    mp_obj_t state[] = {
        mp_obj_new_int(buttons->x - INT16_MAX),
        mp_obj_new_int(buttons->y - INT16_MAX),
        mp_obj_new_int(buttons->z - INT16_MAX),
        mp_obj_new_int(buttons->rz - INT16_MAX),
        mp_obj_new_int(buttons->left_trigger),
        mp_obj_new_int(buttons->right_trigger),
        mp_obj_new_int(buttons->dpad),
        mp_obj_new_int(buttons->buttons),
        mp_obj_new_int(buttons->upload),
        mp_obj_new_int(buttons->profile),
        mp_obj_new_int(buttons->trigger_switches),
        mp_obj_new_int(buttons->paddles),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(state), state);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_state_obj, pb_type_xbox_state);

static mp_obj_t pb_type_xbox_dpad(mp_obj_t self_in) {
    xbox_input_map_t *buttons = pb_type_xbox_get_input(self_in);
    return mp_obj_new_int(buttons->dpad);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_dpad_obj, pb_type_xbox_dpad);

static mp_obj_t pb_type_xbox_profile(mp_obj_t self_in) {
    xbox_input_map_t *buttons = pb_type_xbox_get_input(self_in);
    return mp_obj_new_int(buttons->profile);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_profile_obj, pb_type_xbox_profile);

static mp_obj_t pb_type_xbox_joystick(mp_obj_t self_in, uint16_t x_raw, uint16_t y_raw) {
    pb_type_xbox_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_int_t x = (x_raw - INT16_MAX) * 100 / INT16_MAX;
    mp_int_t y = (INT16_MAX - y_raw) * 100 / INT16_MAX;

    // Apply square deadzone to prevent drift.
    if (x < self->joystick_deadzone && x > -self->joystick_deadzone &&
        y < self->joystick_deadzone && y > -self->joystick_deadzone) {
        x = 0;
        y = 0;
    }

    mp_obj_t directions[] = {
        mp_obj_new_int(x),
        mp_obj_new_int(y),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(directions), directions);
}

static mp_obj_t pb_type_xbox_joystick_left(mp_obj_t self_in) {
    xbox_input_map_t *buttons = pb_type_xbox_get_input(self_in);
    return pb_type_xbox_joystick(self_in, buttons->x, buttons->y);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_joystick_left_obj, pb_type_xbox_joystick_left);

static mp_obj_t pb_type_xbox_joystick_right(mp_obj_t self_in) {
    xbox_input_map_t *buttons = pb_type_xbox_get_input(self_in);
    return pb_type_xbox_joystick(self_in, buttons->z, buttons->rz);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_joystick_right_obj, pb_type_xbox_joystick_right);

static mp_obj_t pb_type_xbox_triggers(mp_obj_t self_in) {
    xbox_input_map_t *buttons = pb_type_xbox_get_input(self_in);
    mp_obj_t tiggers[] = {
        mp_obj_new_int(buttons->left_trigger * 100 / 1023),
        mp_obj_new_int(buttons->right_trigger * 100 / 1023),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(tiggers), tiggers);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_xbox_triggers_obj, pb_type_xbox_triggers);

typedef struct {
    uint8_t activation_flags;
    uint8_t power_left_trigger;
    uint8_t power_right_trigger;
    uint8_t power_left_handle;
    uint8_t power_right_handle;
    uint8_t duration_10ms;
    uint8_t delay_10ms;
    uint8_t repetitions;
} __attribute__((packed)) xbox_rumble_command_t;

static mp_obj_t pb_type_xbox_rumble(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_xbox_obj_t, self,
        PB_ARG_DEFAULT_INT(power, 100),
        PB_ARG_DEFAULT_INT(duration, 200),
        PB_ARG_DEFAULT_INT(count, 1),
        PB_ARG_DEFAULT_INT(delay, 100));

    // 1 unit is 10ms, max duration is 250=2500ms.
    mp_int_t duration = pb_obj_get_positive_int(duration_in) / 10;
    mp_int_t delay = pb_obj_get_positive_int(delay_in) / 10;

    // Number of rumbles, capped at 100.
    mp_int_t count = pb_obj_get_pct(count_in);

    // Don't prefix delay if there is only one rumble.
    if (count == 1) {
        delay = 0;
    }

    // User order is left, right, left trigger, right trigger.
    int8_t intensity[4];
    pb_obj_get_pct_or_array(power_in, 4, intensity);

    // If a single value is given, rumble only the main actuators.
    if (!pb_obj_is_array(power_in)) {
        intensity[2] = 0;
        intensity[3] = 0;
    }

    xbox_rumble_command_t command = {
        .activation_flags = 0,
        .power_left_trigger = intensity[2],
        .power_right_trigger = intensity[3],
        .power_left_handle = intensity[0],
        .power_right_handle = intensity[1],
        .duration_10ms = duration > 250 ? 250 : duration,
        .delay_10ms = delay > 250 ? 250 : delay,
        .repetitions = count - 1,
    };

    if (command.power_right_handle) {
        command.activation_flags |= 0x01;
    }
    if (command.power_left_handle) {
        command.activation_flags |= 0x02;
    }
    if (command.power_right_trigger) {
        command.activation_flags |= 0x04;
    }
    if (command.power_left_trigger) {
        command.activation_flags |= 0x08;
    }

    // If all intensities are zero or duration or number is invalid, do nothing.
    if (command.activation_flags == 0 || count < 1 || duration < 1) {
        return mp_const_none;
    }

    // REVISIT: Discover this handle dynamically.
    const uint16_t handle = 34;

    pbdrv_bluetooth_peripheral_write_characteristic(self->peripheral, handle, (const uint8_t *)&command, sizeof(command));
    return pb_type_xbox_await_operation(MP_OBJ_TO_PTR(self));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_xbox_rumble_obj, 1, pb_type_xbox_rumble);

static const mp_rom_map_elem_t pb_type_xbox_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_type_xbox_name_obj)  },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_type_xbox_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&pb_type_xbox_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&pb_type_xbox_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&pb_type_xbox_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_dpad), MP_ROM_PTR(&pb_type_xbox_dpad_obj) },
    { MP_ROM_QSTR(MP_QSTR_profile), MP_ROM_PTR(&pb_type_xbox_profile_obj) },
    { MP_ROM_QSTR(MP_QSTR_joystick_left), MP_ROM_PTR(&pb_type_xbox_joystick_left_obj) },
    { MP_ROM_QSTR(MP_QSTR_joystick_right), MP_ROM_PTR(&pb_type_xbox_joystick_right_obj) },
    { MP_ROM_QSTR(MP_QSTR_triggers), MP_ROM_PTR(&pb_type_xbox_triggers_obj) },
    { MP_ROM_QSTR(MP_QSTR_rumble), MP_ROM_PTR(&pb_type_xbox_rumble_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_xbox_locals_dict, pb_type_xbox_locals_dict_table);

static const pb_attr_dict_entry_t pb_type_xbox_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, pb_type_xbox_obj_t, buttons),
    PB_ATTR_DICT_SENTINEL
};

MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_XboxController,
    MP_QSTR_XboxController,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_xbox_make_new,
    locals_dict, &pb_type_xbox_locals_dict,
    attr, pb_attribute_handler,
    protocol, pb_type_xbox_attr_dict);

#endif // PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER
