// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/ev3devices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.ev3devices.InfraredSensor class object
typedef struct _ev3devices_InfraredSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} ev3devices_InfraredSensor_obj_t;

// pybricks.ev3devices.InfraredSensor.__init__
STATIC mp_obj_t ev3devices_InfraredSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    ev3devices_InfraredSensor_obj_t *self = m_new_obj(ev3devices_InfraredSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.InfraredSensor.distance
STATIC mp_obj_t ev3devices_InfraredSensor_distance(mp_obj_t self_in) {
    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t distance;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__PROX, &distance);
    return mp_obj_new_int(distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_InfraredSensor_distance_obj, ev3devices_InfraredSensor_distance);

// pybricks.ev3devices.InfraredSensor.beacon
STATIC mp_obj_t ev3devices_InfraredSensor_beacon(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_InfraredSensor_obj_t, self,
        PB_ARG_REQUIRED(channel));

    mp_int_t channel_no = pb_obj_get_int(channel);
    if (channel_no < 1 || channel_no > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    int32_t beacon_data[8];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__SEEK, beacon_data);

    mp_int_t heading = beacon_data[channel_no * 2 - 2] * 3;
    mp_int_t distance = beacon_data[channel_no * 2 - 1];

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
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_InfraredSensor_beacon_obj, 1, ev3devices_InfraredSensor_beacon);

// pybricks.ev3devices.InfraredSensor.buttons
STATIC mp_obj_t ev3devices_InfraredSensor_buttons(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_InfraredSensor_obj_t, self,
        PB_ARG_REQUIRED(channel));

    mp_int_t channel_no = pb_obj_get_int(channel);
    if (channel_no < 1 || channel_no > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    int32_t buttons_data[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REMOTE, buttons_data);

    mp_int_t encoded = buttons_data[channel_no - 1];
    mp_obj_t pressed[2];
    uint8_t len = 0;

    switch (encoded) {
        case 0:
            break;
        case 1:
            pressed[len++] = pb_const_button_left_up;
            break;
        case 2:
            pressed[len++] = pb_const_button_left_down;
            break;
        case 3:
            pressed[len++] = pb_const_button_right_up;
            break;
        case 4:
            pressed[len++] = pb_const_button_right_down;
            break;
        case 5:
            pressed[len++] = pb_const_button_left_up;
            pressed[len++] = pb_const_button_right_up;
            break;
        case 6:
            pressed[len++] = pb_const_button_left_up;
            pressed[len++] = pb_const_button_right_down;
            break;
        case 7:
            pressed[len++] = pb_const_button_left_down;
            pressed[len++] = pb_const_button_right_up;
            break;
        case 8:
            pressed[len++] = pb_const_button_left_down;
            pressed[len++] = pb_const_button_right_down;
            break;
        case 9:
            pressed[len++] = pb_const_button_beacon;
            break;
        case 10:
            pressed[len++] = pb_const_button_left_up;
            pressed[len++] = pb_const_button_left_down;
            break;
        case 11:
            pressed[len++] = pb_const_button_right_up;
            pressed[len++] = pb_const_button_right_down;
            break;
        default:
            pb_assert(PBIO_ERROR_IO);
            break;
    }

    return mp_obj_new_list(len, pressed);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_InfraredSensor_buttons_obj, 1, ev3devices_InfraredSensor_buttons);

// pybricks.ev3devices.InfraredSensor.keypad
STATIC mp_obj_t ev3devices_InfraredSensor_keypad(mp_obj_t self_in) {

    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t keypad_data;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REM_A, &keypad_data);

    if (keypad_data == 384) {
        return mp_obj_new_list(0, NULL);
    }

    mp_obj_t pressed[4];
    uint8_t len = 0;

    if (keypad_data & 0x10) {
        pressed[len++] = pb_const_button_left_up;
    }
    if (keypad_data & 0x20) {
        pressed[len++] = pb_const_button_left_down;
    }
    if (keypad_data & 0x40) {
        pressed[len++] = pb_const_button_right_up;
    }
    if (keypad_data & 0x80) {
        pressed[len++] = pb_const_button_right_down;
    }

    return mp_obj_new_list(len, pressed);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_InfraredSensor_keypad_obj, ev3devices_InfraredSensor_keypad);

// dir(pybricks.ev3devices.InfraredSensor)
STATIC const mp_rom_map_elem_t ev3devices_InfraredSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&ev3devices_InfraredSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_beacon),   MP_ROM_PTR(&ev3devices_InfraredSensor_beacon_obj) },
    { MP_ROM_QSTR(MP_QSTR_buttons),  MP_ROM_PTR(&ev3devices_InfraredSensor_buttons_obj) },
    { MP_ROM_QSTR(MP_QSTR_keypad),   MP_ROM_PTR(&ev3devices_InfraredSensor_keypad_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_InfraredSensor_locals_dict, ev3devices_InfraredSensor_locals_dict_table);

// type(pybricks.ev3devices.InfraredSensor)
const mp_obj_type_t pb_type_ev3devices_InfraredSensor = {
    { &mp_type_type },
    .name = MP_QSTR_InfraredSensor,
    .make_new = ev3devices_InfraredSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&ev3devices_InfraredSensor_locals_dict,
};

#endif // PYBRICKS_PY_EV3DEVICES
