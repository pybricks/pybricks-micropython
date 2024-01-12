// Copyright (C) 2024 The Pybricks Authors - All rights reserved

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_PUPDEVICES

#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>
#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/error.h>
#include <pbio/task.h>

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

/**
 * The main HID Characteristic.
 */
static pbdrv_bluetooth_peripheral_char_t pb_xbox_char = {
    .handle = 0, // Will be set during discovery.
    .properties = 0x12, // Needed to distingish it from another char with same UUID.
    .uuid16 = 0x2a4d,
    .uuid128 = { 0 },
    .request_notification = true,
};

/**
 * Unused characteristic that needs to be read for controller to become active.
 */
static pbdrv_bluetooth_peripheral_char_t pb_xbox_char_a = {
    .uuid16 = 0x2a4a,
    .request_notification = false,
};

/**
 * Unused characteristic that needs to be read for controller to become active.
 */
static pbdrv_bluetooth_peripheral_char_t pb_xbox_char_b = {
    .uuid16 = 0x2a4b,
    .request_notification = false,
};

// Decoding via https://github.com/esp32beans/ESP32-BLE-HID-exp, MIT license.
typedef struct __attribute__((packed)) {
    uint16_t x; // 0..65534
    uint16_t y; // 0..65534
    uint16_t z; // 0..65534
    uint16_t rz; // 0..65534
    uint16_t brake : 10; // 0..1023
    uint16_t filler1 : 6;
    uint16_t accelerator : 10; // 0..1023
    uint16_t filler2 : 6;
    uint8_t hat : 4;
    uint8_t filler3 : 4;
    uint16_t buttons : 15;
    uint16_t filler4 : 1;
    uint8_t record : 1;
    uint8_t filler5 : 7;
} xbox_one_gamepad_t;

typedef struct {
    pbio_task_t task;
    xbox_one_gamepad_t state;
} pb_xbox_t;

STATIC pb_xbox_t pb_xbox_singleton;

// Handles LEGO Wireless protocol messages from the XBOX Device.
STATIC pbio_pybricks_error_t handle_notification(pbdrv_bluetooth_connection_t connection, const uint8_t *value, uint32_t size) {
    pb_xbox_t *xbox = &pb_xbox_singleton;
    if (size == sizeof(xbox_one_gamepad_t)) {
        memcpy(&xbox->state, &value[0], (size < sizeof(xbox_one_gamepad_t)) ? size : sizeof(xbox_one_gamepad_t));
    }
    return PBIO_PYBRICKS_ERROR_OK;
}

#define _16BIT_AS_LE(x) ((x) & 0xff), (((x) >> 8) & 0xff)

STATIC pbdrv_bluetooth_ad_match_result_flags_t xbox_advertisement_matches(uint8_t event_type, const uint8_t *data, const char *name, const uint8_t *addr, const uint8_t *match_addr) {

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

    // As above, but without discovery mode.
    uint8_t advertising_data3[sizeof(advertising_data2)];
    memcpy(advertising_data3, advertising_data2, sizeof(advertising_data2));
    advertising_data3[2] = 0x04;

    // Exit if neither of the expected values match.
    if (memcmp(data, advertising_data1, sizeof(advertising_data1)) &&
        memcmp(data, advertising_data2, sizeof(advertising_data2)) &&
        memcmp(data, advertising_data3, sizeof(advertising_data3))) {
        return PBDRV_BLUETOOTH_AD_MATCH_NONE;
    }

    // Expected value matches at this point.
    pbdrv_bluetooth_ad_match_result_flags_t flags = PBDRV_BLUETOOTH_AD_MATCH_VALUE;

    // Compare address in advertisement to previously scanned address.
    if (memcmp(addr, match_addr, 6) == 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_ADDRESS;
    }
    return flags;
}

STATIC pbdrv_bluetooth_ad_match_result_flags_t xbox_advertisement_response_matches(uint8_t event_type, const uint8_t *data, const char *name, const uint8_t *addr, const uint8_t *match_addr) {

    pbdrv_bluetooth_ad_match_result_flags_t flags = PBDRV_BLUETOOTH_AD_MATCH_NONE;

    // This is currently the only requirement.
    if (event_type == PBDRV_BLUETOOTH_AD_TYPE_SCAN_RSP) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_VALUE;
    }

    // Compare address in response to previously scanned address.
    if (memcmp(addr, match_addr, 6) == 0) {
        flags |= PBDRV_BLUETOOTH_AD_MATCH_ADDRESS;
    }

    return flags;
}

STATIC void pb_xbox_assert_connected(void) {
    if (!pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        mp_raise_OSError(MP_ENODEV);
    }
}

typedef struct _pb_type_xbox_obj_t {
    mp_obj_base_t base;
} pb_type_xbox_obj_t;

STATIC mp_obj_t pb_type_xbox_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_type_xbox_obj_t *self = mp_obj_malloc(pb_type_xbox_obj_t, type);

    mp_int_t timeout = timeout_in == mp_const_none ? -1 : pb_obj_get_positive_int(timeout_in);

    pb_xbox_t *xbox = &pb_xbox_singleton;

    // REVISIT: for now, we only allow a single connection to a XBOX device.
    if (pbdrv_bluetooth_is_connected(PBDRV_BLUETOOTH_CONNECTION_PERIPHERAL)) {
        pb_assert(PBIO_ERROR_BUSY);
    }

    // HACK: scan and connect may block sending other Bluetooth messages, so we
    // need to make sure the stdout queue is drained first to avoid unexpected
    // behavior
    mp_hal_stdout_tx_flush();

    // needed to ensure that no buttons are "pressed" after reconnecting since
    // we are using static memory
    memset(xbox, 0, sizeof(*xbox));
    xbox->state.x = xbox->state.y = xbox->state.z = xbox->state.rz = INT16_MAX;

    // Connect with bonding enabled.
    pbdrv_bluetooth_peripheral_scan_and_connect(&xbox->task, xbox_advertisement_matches, xbox_advertisement_response_matches, handle_notification, true);
    pb_module_tools_pbio_task_do_blocking(&xbox->task, timeout);

    // Discover and read char A but discard the result.
    pbdrv_bluetooth_periperal_discover_characteristic(&xbox->task, &pb_xbox_char_a);
    pb_module_tools_pbio_task_do_blocking(&xbox->task, timeout);
    pbdrv_bluetooth_periperal_read_characteristic(&xbox->task, &pb_xbox_char_a);
    pb_module_tools_pbio_task_do_blocking(&xbox->task, timeout);

    // Discover and read char B but discard the result.
    pbdrv_bluetooth_periperal_discover_characteristic(&xbox->task, &pb_xbox_char_b);
    pb_module_tools_pbio_task_do_blocking(&xbox->task, timeout);
    pbdrv_bluetooth_periperal_read_characteristic(&xbox->task, &pb_xbox_char_b);
    pb_module_tools_pbio_task_do_blocking(&xbox->task, timeout);

    // Discover and read char D but discard the result, and enable notifications.
    pbdrv_bluetooth_periperal_discover_characteristic(&xbox->task, &pb_xbox_char);
    pb_module_tools_pbio_task_do_blocking(&xbox->task, timeout);
    pbdrv_bluetooth_periperal_read_characteristic(&xbox->task, &pb_xbox_char);
    pb_module_tools_pbio_task_do_blocking(&xbox->task, timeout);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t pb_xbox_name(size_t n_args, const mp_obj_t *args) {
    // pb_xbox_t *xbox = &pb_xbox_singleton;
    pb_xbox_assert_connected();
    const char *name = pbdrv_bluetooth_peripheral_get_name();
    return mp_obj_new_str(name, strlen(name));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pb_xbox_name_obj, 1, 2, pb_xbox_name);

STATIC mp_obj_t pb_xbox_state(mp_obj_t self_in) {
    mp_obj_t state[] = {
        mp_obj_new_int(pb_xbox_singleton.state.x - INT16_MAX),
        mp_obj_new_int(pb_xbox_singleton.state.y - INT16_MAX),
        mp_obj_new_int(pb_xbox_singleton.state.z - INT16_MAX),
        mp_obj_new_int(pb_xbox_singleton.state.rz - INT16_MAX),
        mp_obj_new_int(pb_xbox_singleton.state.brake),
        mp_obj_new_int(pb_xbox_singleton.state.accelerator),
        mp_obj_new_int(pb_xbox_singleton.state.hat),
        mp_obj_new_int(pb_xbox_singleton.state.buttons),
        mp_obj_new_int(pb_xbox_singleton.state.record),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(state), state);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_xbox_state_obj, pb_xbox_state);

STATIC const mp_rom_map_elem_t pb_type_xbox_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_name),  MP_ROM_PTR(&pb_xbox_name_obj)   },
    { MP_ROM_QSTR(MP_QSTR_state), MP_ROM_PTR(&pb_xbox_state_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_xbox_locals_dict, pb_type_xbox_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_XboxController,
    MP_QSTR_XboxController,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_xbox_make_new,
    locals_dict, &pb_type_xbox_locals_dict);

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_PUPDEVICES
