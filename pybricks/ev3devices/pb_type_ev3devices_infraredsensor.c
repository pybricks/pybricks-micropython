// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/ev3devices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.ev3devices.InfraredSensor class object
typedef struct _ev3devices_InfraredSensor_obj_t {
    pb_type_device_obj_base_t device_base;
} ev3devices_InfraredSensor_obj_t;

// pybricks.ev3devices.InfraredSensor.__init__
static mp_obj_t ev3devices_InfraredSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    ev3devices_InfraredSensor_obj_t *self = mp_obj_malloc(ev3devices_InfraredSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_EV3_IR_SENSOR);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.InfraredSensor.distance
static mp_obj_t ev3devices_InfraredSensor_distance(mp_obj_t self_in) {
    int8_t *distance = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__PROX);
    return mp_obj_new_int(distance[0]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_InfraredSensor_distance_obj, ev3devices_InfraredSensor_distance);

// pybricks.ev3devices.InfraredSensor.beacon
static mp_obj_t ev3devices_InfraredSensor_beacon(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_InfraredSensor_obj_t, self,
        PB_ARG_REQUIRED(channel));

    mp_int_t channel = pb_obj_get_int(channel_in);
    if (channel < 1 || channel > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    int8_t *beacon_data = pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__SEEK);

    mp_int_t heading = beacon_data[channel * 2 - 2] * 3;
    mp_int_t distance = beacon_data[channel * 2 - 1];

    mp_obj_t ret[2];

    if (distance == -128) {
        ret[0] = mp_const_none;
        ret[1] = mp_const_none;
    } else {
        ret[0] = mp_obj_new_int(distance);
        ret[1] = mp_obj_new_int(heading);
    }

    return mp_obj_new_tuple(2, ret);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_InfraredSensor_beacon_obj, 1, ev3devices_InfraredSensor_beacon);

// pybricks.ev3devices.InfraredSensor.buttons
static mp_obj_t ev3devices_InfraredSensor_buttons(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_InfraredSensor_obj_t, self,
        PB_ARG_REQUIRED(channel));

    mp_int_t channel = pb_obj_get_int(channel_in);
    if (channel < 1 || channel > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    int8_t *buttons_data = pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__REMOTE);

    mp_int_t encoded = buttons_data[channel - 1];
    mp_obj_t pressed[2];
    uint8_t len = 0;

    switch (encoded) {
        case 0:
            break;
        case 1:
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_UP);
            break;
        case 2:
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_DOWN);
            break;
        case 3:
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_UP);
            break;
        case 4:
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_DOWN);
            break;
        case 5:
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_UP);
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_UP);
            break;
        case 6:
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_UP);
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_DOWN);
            break;
        case 7:
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_DOWN);
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_UP);
            break;
        case 8:
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_DOWN);
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_DOWN);
            break;
        case 9:
            pressed[len++] = pb_type_button_new(MP_QSTR_BEACON);
            break;
        case 10:
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_UP);
            pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_DOWN);
            break;
        case 11:
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_UP);
            pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_DOWN);
            break;
        default:
            pb_assert(PBIO_ERROR_IO);
            break;
    }

    return mp_obj_new_list(len, pressed);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_InfraredSensor_buttons_obj, 1, ev3devices_InfraredSensor_buttons);

// pybricks.ev3devices.InfraredSensor.keypad
static mp_obj_t ev3devices_InfraredSensor_keypad(mp_obj_t self_in) {

    int16_t keypad_data = *(int16_t *)pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_EV3_INFRARED_SENSOR__REM_A);

    if (keypad_data == 384) {
        return mp_obj_new_list(0, NULL);
    }

    mp_obj_t pressed[4];
    uint8_t len = 0;

    if (keypad_data & 0x10) {
        pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_UP);
    }
    if (keypad_data & 0x20) {
        pressed[len++] = pb_type_button_new(MP_QSTR_LEFT_UP);
    }
    if (keypad_data & 0x40) {
        pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_UP);
    }
    if (keypad_data & 0x80) {
        pressed[len++] = pb_type_button_new(MP_QSTR_RIGHT_DOWN);
    }

    return mp_obj_new_list(len, pressed);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_InfraredSensor_keypad_obj, ev3devices_InfraredSensor_keypad);

// dir(pybricks.ev3devices.InfraredSensor)
static const mp_rom_map_elem_t ev3devices_InfraredSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&ev3devices_InfraredSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_beacon),   MP_ROM_PTR(&ev3devices_InfraredSensor_beacon_obj) },
    { MP_ROM_QSTR(MP_QSTR_buttons),  MP_ROM_PTR(&ev3devices_InfraredSensor_buttons_obj) },
    { MP_ROM_QSTR(MP_QSTR_keypad),   MP_ROM_PTR(&ev3devices_InfraredSensor_keypad_obj) },
};
static MP_DEFINE_CONST_DICT(ev3devices_InfraredSensor_locals_dict, ev3devices_InfraredSensor_locals_dict_table);

// type(pybricks.ev3devices.InfraredSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_ev3devices_InfraredSensor,
    MP_QSTR_InfraredSensor,
    MP_TYPE_FLAG_NONE,
    make_new, ev3devices_InfraredSensor_make_new,
    locals_dict, &ev3devices_InfraredSensor_locals_dict);

#endif // PYBRICKS_PY_EV3DEVICES
