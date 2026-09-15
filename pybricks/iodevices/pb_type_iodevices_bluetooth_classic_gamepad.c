// SPDX-License-Identifier: MIT
// Copyright (c) 2026 The Pybricks Authors

// Generic Bluetooth Classic HID gamepad.
//
// This only exposes the raw HID input report. Decoding it into buttons and
// axes is device specific and left to Python subclasses.

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES_BLUETOOTH_CLASSIC_GAMEPAD

#include <stdint.h>
#include <string.h>

#include <pbdrv/bluetooth.h>

#include "iodevices.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"

typedef struct _pb_type_bluetooth_classic_gamepad_obj_t {
    mp_obj_base_t base;
} pb_type_bluetooth_classic_gamepad_obj_t;

static void pb_type_bluetooth_classic_gamepad_assert_connected(void) {
    if (!pbdrv_bluetooth_classic_hid_is_connected()) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT(
            "Please pair the controller in the hub menu first "
            "and make sure the controller is on."
            ));
    }
}

static mp_obj_t pb_type_bluetooth_classic_gamepad_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // Pairing and connecting is handled by the hub menu, so all this class
    // does is verify that the device is there.
    pb_type_bluetooth_classic_gamepad_assert_connected();

    return MP_OBJ_FROM_PTR(mp_obj_malloc(pb_type_bluetooth_classic_gamepad_obj_t, type));
}

static mp_obj_t pb_type_bluetooth_classic_gamepad_name(mp_obj_t self_in) {
    pb_type_bluetooth_classic_gamepad_assert_connected();
    const char *name = pbdrv_bluetooth_classic_hid_get_connected_name();
    if (!name) {
        return mp_const_none;
    }
    return mp_obj_new_str(name, strlen(name));
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_bluetooth_classic_gamepad_name_obj, pb_type_bluetooth_classic_gamepad_name);

static mp_obj_t pb_type_bluetooth_classic_gamepad_report(mp_obj_t self_in, mp_obj_t report_id_in) {

    pb_type_bluetooth_classic_gamepad_assert_connected();

    uint8_t report_id = mp_obj_get_int(report_id_in);

    uint8_t report[PBDRV_BLUETOOTH_HID_MAX_REPORT_SIZE];
    uint32_t size = pbdrv_bluetooth_classic_hid_get_report(report_id, report, sizeof(report));

    if (!size) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT(
            "This controller does not send this report."
            ));
    }

    return mp_obj_new_bytes(report, size);
}
static MP_DEFINE_CONST_FUN_OBJ_2(pb_type_bluetooth_classic_gamepad_report_obj, pb_type_bluetooth_classic_gamepad_report);

static mp_obj_t pb_type_bluetooth_classic_gamepad_report_ids(mp_obj_t self_in) {

    pb_type_bluetooth_classic_gamepad_assert_connected();

    mp_obj_t ids[PBDRV_BLUETOOTH_HID_NUM_REPORTS];
    size_t count = 0;

    for (uint32_t i = 0; i < PBDRV_BLUETOOTH_HID_NUM_REPORTS; i++) {
        uint8_t report_id;
        if (pbdrv_bluetooth_classic_hid_get_report_id(i, &report_id)) {
            ids[count++] = MP_OBJ_NEW_SMALL_INT(report_id);
        }
    }

    return mp_obj_new_tuple(count, ids);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_bluetooth_classic_gamepad_report_ids_obj, pb_type_bluetooth_classic_gamepad_report_ids);

static const mp_rom_map_elem_t pb_type_bluetooth_classic_gamepad_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&pb_type_bluetooth_classic_gamepad_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_report), MP_ROM_PTR(&pb_type_bluetooth_classic_gamepad_report_obj) },
    { MP_ROM_QSTR(MP_QSTR_report_ids), MP_ROM_PTR(&pb_type_bluetooth_classic_gamepad_report_ids_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_bluetooth_classic_gamepad_locals_dict, pb_type_bluetooth_classic_gamepad_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_BluetoothClassicGamepad,
    MP_QSTR_BluetoothClassicGamepad,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_bluetooth_classic_gamepad_make_new,
    locals_dict, &pb_type_bluetooth_classic_gamepad_locals_dict);

#endif // PYBRICKS_PY_IODEVICES_BLUETOOTH_CLASSIC_GAMEPAD
