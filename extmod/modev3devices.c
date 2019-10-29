// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include "py/mpconfig.h"

#include "modmotor.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "pbobj.h"
#include "pbkwarg.h"
#include "modlight.h"
#include "modparameters.h"

#include "py/objtype.h"

#include <pbio/iodev.h>
#include <pbio/light.h>
#include <pbio/button.h>
#include <pbio/ev3device.h>
#include <pberror.h>

// pybricks.ev3devices.TouchSensor class object
typedef struct _ev3devices_TouchSensor_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
} ev3devices_TouchSensor_obj_t;

// pybricks.ev3devices.TouchSensor.__init__
STATIC mp_obj_t ev3devices_TouchSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    ev3devices_TouchSensor_obj_t *self = m_new_obj(ev3devices_TouchSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_TOUCH_SENSOR, port_num));
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.TouchSensor.__str__
STATIC void ev3devices_TouchSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_TouchSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.ev3devices.TouchSensor.pressed
STATIC mp_obj_t ev3devices_TouchSensor_pressed(mp_obj_t self_in) {
    ev3devices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_TOUCH_SENSOR__TOUCH, &analog));
    return mp_obj_new_bool(analog > 250);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_TouchSensor_pressed_obj, ev3devices_TouchSensor_pressed);


// dir(pybricks.ev3devices.TouchSensor)
STATIC const mp_rom_map_elem_t ev3devices_TouchSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&ev3devices_TouchSensor_pressed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_TouchSensor_locals_dict, ev3devices_TouchSensor_locals_dict_table);

// type(pybricks.ev3devices.TouchSensor)
STATIC const mp_obj_type_t ev3devices_TouchSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_TouchSensor,
    .print = ev3devices_TouchSensor_print,
    .make_new = ev3devices_TouchSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_TouchSensor_locals_dict,
};

// pybricks.ev3devices.InfraredSensor class object
typedef struct _ev3devices_InfraredSensor_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
} ev3devices_InfraredSensor_obj_t;

// pybricks.ev3devices.InfraredSensor.__init__
STATIC mp_obj_t ev3devices_InfraredSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    ev3devices_InfraredSensor_obj_t *self = m_new_obj(ev3devices_InfraredSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR, port_num));
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.InfraredSensor.__str__
STATIC void ev3devices_InfraredSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_InfraredSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.ev3devices.InfraredSensor.distance
STATIC mp_obj_t ev3devices_InfraredSensor_distance(mp_obj_t self_in) {
    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t distance;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__PROX, &distance));
    return mp_obj_new_int(distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_InfraredSensor_distance_obj, ev3devices_InfraredSensor_distance);

// pybricks.ev3devices.InfraredSensor.beacon
STATIC mp_obj_t ev3devices_InfraredSensor_beacon(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(channel)
    );

    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_int_t channel_no = pb_obj_get_int(channel);
    if (channel_no < 1 || channel_no > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    int8_t beacon_data[8];
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__SEEK, beacon_data));

    mp_int_t heading = beacon_data[channel_no*2-2]*3;
    mp_int_t distance = beacon_data[channel_no*2-1];

    mp_obj_t ret[2];

    if (distance == -128) {
        ret[0] = mp_const_none;
        ret[1] = mp_const_none;
    }
    else {
        ret[0] = mp_obj_new_int(distance);
        ret[1] = mp_obj_new_int(heading);
    }

    return mp_obj_new_tuple(2, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_InfraredSensor_beacon_obj, 0, ev3devices_InfraredSensor_beacon);

// pybricks.ev3devices.InfraredSensor.buttons
STATIC mp_obj_t ev3devices_InfraredSensor_buttons(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(channel)
    );

    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_int_t channel_no = pb_obj_get_int(channel);
    if (channel_no < 1 || channel_no > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    int8_t buttons_data[4];
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REMOTE, buttons_data));

    mp_int_t encoded = buttons_data[channel_no-1];
    mp_int_t pressed[2];
    mp_obj_t pressed_obj[2];
    uint8_t len = 0;

    switch(encoded) {
        case 0:
            break;
        case 1:
            pressed[len++] = PBIO_BUTTON_LEFT_UP;
            break;
        case 2:
            pressed[len++] = PBIO_BUTTON_LEFT_DOWN;
            break;
        case 3:
            pressed[len++] = PBIO_BUTTON_RIGHT_UP;
            break;
        case 4:
            pressed[len++] = PBIO_BUTTON_RIGHT_DOWN;
            break;
        case 5:
            pressed[len++] = PBIO_BUTTON_LEFT_UP;
            pressed[len++] = PBIO_BUTTON_RIGHT_UP;
            break;
        case 6:
            pressed[len++] = PBIO_BUTTON_LEFT_UP;
            pressed[len++] = PBIO_BUTTON_RIGHT_DOWN;
            break;
        case 7:
            pressed[len++] = PBIO_BUTTON_LEFT_DOWN;
            pressed[len++] = PBIO_BUTTON_RIGHT_UP;
            break;
        case 8:
            pressed[len++] = PBIO_BUTTON_LEFT_DOWN;
            pressed[len++] = PBIO_BUTTON_RIGHT_DOWN;
            break;
        case 10:
            pressed[len++] = PBIO_BUTTON_LEFT_UP;
            pressed[len++] = PBIO_BUTTON_LEFT_DOWN;
            break;
        case 11:
            pressed[len++] = PBIO_BUTTON_RIGHT_UP;
            pressed[len++] = PBIO_BUTTON_RIGHT_DOWN;
            break;
        case 9:
            pressed[len++] = PBIO_BUTTON_UP;
            break;
        default:
            pb_assert(PBIO_ERROR_IO);
            break;
    }

    for (uint8_t i = 0; i < len; i++) {
        pressed_obj[i] = MP_OBJ_NEW_SMALL_INT(pressed[i]);
    }
    return mp_obj_new_list(len, pressed_obj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_InfraredSensor_buttons_obj, 0, ev3devices_InfraredSensor_buttons);

// pybricks.ev3devices.InfraredSensor.keypad
STATIC mp_obj_t ev3devices_InfraredSensor_keypad(mp_obj_t self_in) {

    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int16_t keypad_data;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_INFRARED_SENSOR__REM_A, &keypad_data));

    mp_obj_t pressed_obj[4];
    uint8_t len = 0;

    if (keypad_data == 384) {
        return mp_obj_new_list(0, pressed_obj);
    } else {
        if (keypad_data & 0x10) {
            pressed_obj[len++] = MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_LEFT_UP);
        }
        if (keypad_data & 0x20) {
            pressed_obj[len++] = MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_LEFT_DOWN);
        }
        if (keypad_data & 0x40) {
            pressed_obj[len++] = MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_RIGHT_UP);
        }
        if (keypad_data & 0x80) {
            pressed_obj[len++] = MP_OBJ_NEW_SMALL_INT(PBIO_BUTTON_RIGHT_DOWN);
        }
    }

    return mp_obj_new_list(len, pressed_obj);
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
STATIC const mp_obj_type_t ev3devices_InfraredSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_InfraredSensor,
    .print = ev3devices_InfraredSensor_print,
    .make_new = ev3devices_InfraredSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_InfraredSensor_locals_dict,
};

// pybricks.ev3devices.ColorSensor class object
typedef struct _ev3devices_ColorSensor_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
} ev3devices_ColorSensor_obj_t;

// pybricks.ev3devices.ColorSensor.__init__
STATIC mp_obj_t ev3devices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );
    ev3devices_ColorSensor_obj_t *self = m_new_obj(ev3devices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR, port_num));
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.ColorSensor.__str__
STATIC void ev3devices_ColorSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_ColorSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.ev3devices.ColorSensor.color
STATIC mp_obj_t ev3devices_ColorSensor_color(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t color;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__COLOR, &color));

    switch(color) {
        case 1: return MP_OBJ_FROM_PTR(&pb_const_black);
        case 2: return MP_OBJ_FROM_PTR(&pb_const_blue);
        case 3: return MP_OBJ_FROM_PTR(&pb_const_green);
        case 4: return MP_OBJ_FROM_PTR(&pb_const_yellow);
        case 5: return MP_OBJ_FROM_PTR(&pb_const_red);
        case 6: return MP_OBJ_FROM_PTR(&pb_const_white);
        case 7: return MP_OBJ_FROM_PTR(&pb_const_brown);
        default: return mp_const_none;
    }
    return mp_obj_new_int(color);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_color_obj, ev3devices_ColorSensor_color);

// pybricks.ev3devices.ColorSensor.ambient
STATIC mp_obj_t ev3devices_ColorSensor_ambient(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t ambient;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__AMBIENT, &ambient));
    return mp_obj_new_int(ambient);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_ambient_obj, ev3devices_ColorSensor_ambient);

// pybricks.ev3devices.ColorSensor.reflection
STATIC mp_obj_t ev3devices_ColorSensor_reflection(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t reflection;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__REFLECT, &reflection));
    return mp_obj_new_int(reflection);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_reflection_obj, ev3devices_ColorSensor_reflection);

// pybricks.ev3devices.ColorSensor.rgb
STATIC mp_obj_t ev3devices_ColorSensor_rgb(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t rgb[3];
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__RGB_RAW, rgb));
    mp_obj_t tup[3];

    rgb[0] = (0.258*rgb[0])-0.3;
    rgb[1] = (0.280*rgb[1])-0.8;
    rgb[2] = (0.523*rgb[2])-3.7;

    for (uint8_t i = 0; i < 3; i++) {
        rgb[i] = (rgb[i] > 100 ? 100 : rgb[i]);
        rgb[i] = (rgb[i] < 0   ?   0 : rgb[i]);
        tup[i] = mp_obj_new_int(rgb[i]);
    }
    return mp_obj_new_tuple(3, tup);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_rgb_obj, ev3devices_ColorSensor_rgb);

// dir(pybricks.ev3devices.ColorSensor)
STATIC const mp_rom_map_elem_t ev3devices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&ev3devices_ColorSensor_reflection_obj) },
    { MP_ROM_QSTR(MP_QSTR_ambient   ), MP_ROM_PTR(&ev3devices_ColorSensor_ambient_obj)    },
    { MP_ROM_QSTR(MP_QSTR_color     ), MP_ROM_PTR(&ev3devices_ColorSensor_color_obj)      },
    { MP_ROM_QSTR(MP_QSTR_rgb       ), MP_ROM_PTR(&ev3devices_ColorSensor_rgb_obj)        },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_ColorSensor_locals_dict, ev3devices_ColorSensor_locals_dict_table);

// type(pybricks.ev3devices.ColorSensor)
STATIC const mp_obj_type_t ev3devices_ColorSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorSensor,
    .print = ev3devices_ColorSensor_print,
    .make_new = ev3devices_ColorSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_ColorSensor_locals_dict,
};

// pybricks.ev3devices.UltrasonicSensor class object
typedef struct _ev3devices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    pbio_ev3iodev_t *iodev;
} ev3devices_UltrasonicSensor_obj_t;


// pybricks.ev3devices.UltrasonicSensor.__init__
STATIC mp_obj_t ev3devices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    ev3devices_UltrasonicSensor_obj_t *self = m_new_obj(ev3devices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR, port_num));

    // Create an instance of the Light class
    self->light = ev3devices_Light_obj_make_new(self->iodev);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.UltrasonicSensor.__str__
STATIC void ev3devices_UltrasonicSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_UltrasonicSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.ev3devices.UltrasonicSensor.distance
STATIC mp_obj_t ev3devices_UltrasonicSensor_distance(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_FALSE(silent)
    );
    ev3devices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    int16_t distance;
    if (mp_obj_is_true(silent)) {
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM, &distance));
    }
    else {
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM, &distance));
    }
    return mp_obj_new_int(distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_UltrasonicSensor_distance_obj, 0, ev3devices_UltrasonicSensor_distance);

// pybricks.ev3devices.UltrasonicSensor.presence
STATIC mp_obj_t ev3devices_UltrasonicSensor_presence(mp_obj_t self_in) {
    ev3devices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t presence;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__LISTEN, &presence));
    return mp_obj_new_bool(presence);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_UltrasonicSensor_presence_obj, ev3devices_UltrasonicSensor_presence);

// dir(pybricks.ev3devices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t ev3devices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&ev3devices_UltrasonicSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_presence), MP_ROM_PTR(&ev3devices_UltrasonicSensor_presence_obj) },
    { MP_ROM_QSTR(MP_QSTR_light), MP_ROM_ATTRIBUTE_OFFSET(ev3devices_UltrasonicSensor_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_UltrasonicSensor_locals_dict, ev3devices_UltrasonicSensor_locals_dict_table);

// type(pybricks.ev3devices.UltrasonicSensor)
STATIC const mp_obj_type_t ev3devices_UltrasonicSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_UltrasonicSensor,
    .print = ev3devices_UltrasonicSensor_print,
    .make_new = ev3devices_UltrasonicSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_UltrasonicSensor_locals_dict,
};

// pybricks.ev3devices.GyroSensor class object
typedef struct _ev3devices_GyroSensor_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
    pbio_direction_t direction;
    mp_int_t offset;
} ev3devices_GyroSensor_obj_t;

// pybricks.ev3devices.GyroSensor (internal) Get value0 and value1 for G&A mode
STATIC void ev3devices_GyroSensor_raw(pbio_ev3iodev_t *iodev, mp_int_t *raw_angle, mp_int_t *raw_speed) {
    int16_t angle_and_speed[2];
    pb_assert(ev3device_get_values_at_mode(iodev, PBIO_IODEV_MODE_EV3_GYRO_SENSOR__G_A, angle_and_speed));
    *raw_angle = (mp_int_t) angle_and_speed[0];
    *raw_speed = (mp_int_t) angle_and_speed[1];
}

// pybricks.ev3devices.GyroSensor (internal) Get new offset  for new reset angle
STATIC mp_int_t ev3devices_GyroSensor_get_angle_offset(pbio_ev3iodev_t *iodev, pbio_direction_t direction, mp_int_t new_angle) {
    // Read raw sensor values
    mp_int_t raw_angle, raw_speed;
    ev3devices_GyroSensor_raw(iodev, &raw_angle, &raw_speed);

    // Get new offset using arguments and raw values
    if (direction == PBIO_DIRECTION_CLOCKWISE) {
        return raw_angle - new_angle;
    }
    else {
        return -raw_angle - new_angle;
    }
}

// pybricks.ev3devices.GyroSensor.__init__
STATIC mp_obj_t ev3devices_GyroSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_ENUM(direction, pb_const_clockwise)
    );

    ev3devices_GyroSensor_obj_t *self = m_new_obj(ev3devices_GyroSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    self->direction = enum_get_value_maybe(direction, &pb_enum_type_Direction);

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_GYRO_SENSOR, port_num));
    self->offset = ev3devices_GyroSensor_get_angle_offset(self->iodev, self->direction, 0);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.GyroSensor.speed
STATIC mp_obj_t ev3devices_GyroSensor_speed(mp_obj_t self_in) {
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t raw_angle, raw_speed;
    ev3devices_GyroSensor_raw(self->iodev, &raw_angle, &raw_speed);

    if (self->direction == PBIO_DIRECTION_CLOCKWISE) {
        return mp_obj_new_int(raw_speed);
    }
    else {
        return mp_obj_new_int(-raw_speed);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_GyroSensor_speed_obj, ev3devices_GyroSensor_speed);

// pybricks.ev3devices.GyroSensor.angle
STATIC mp_obj_t ev3devices_GyroSensor_angle(mp_obj_t self_in) {
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t raw_angle, raw_speed;
    ev3devices_GyroSensor_raw(self->iodev, &raw_angle, &raw_speed);

    if (self->direction == PBIO_DIRECTION_CLOCKWISE) {
        return mp_obj_new_int(raw_angle - self->offset);
    }
    else {
        return mp_obj_new_int(-raw_angle - self->offset);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_GyroSensor_angle_obj, ev3devices_GyroSensor_angle);

// pybricks.ev3devices.GyroSensor.reset_angle
STATIC mp_obj_t ev3devices_GyroSensor_reset_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(angle)
    );
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    self->offset = ev3devices_GyroSensor_get_angle_offset(self->iodev, self->direction, pb_obj_get_int(angle));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_GyroSensor_reset_angle_obj, 0, ev3devices_GyroSensor_reset_angle);

// pybricks.ev3devices.GyroSensor.__str__
STATIC void ev3devices_GyroSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_GyroSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// dir(pybricks.ev3devices.GyroSensor)
STATIC const mp_rom_map_elem_t ev3devices_GyroSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_angle),       MP_ROM_PTR(&ev3devices_GyroSensor_angle_obj)       },
    { MP_ROM_QSTR(MP_QSTR_speed),       MP_ROM_PTR(&ev3devices_GyroSensor_speed_obj)       },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&ev3devices_GyroSensor_reset_angle_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_GyroSensor_locals_dict, ev3devices_GyroSensor_locals_dict_table);

// type(pybricks.ev3devices.GyroSensor)
STATIC const mp_obj_type_t ev3devices_GyroSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_GyroSensor,
    .print = ev3devices_GyroSensor_print,
    .make_new = ev3devices_GyroSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_GyroSensor_locals_dict,
};

// pybricks.ev3devices.AnalogSensor class object
typedef struct _ev3devices_AnalogSensor_obj_t {
    mp_obj_base_t base;
    bool active;
    pbio_ev3iodev_t *iodev;
} ev3devices_AnalogSensor_obj_t;

/* Analog Mode enum */

const mp_obj_type_t pb_enum_type_AnalogType;

const pb_obj_enum_elem_t pb_const_ev3 = {
    {&pb_enum_type_AnalogType},
    .name = MP_QSTR_EV3,
    .value = PBIO_IODEV_TYPE_ID_EV3_ANALOG
};

const pb_obj_enum_elem_t pb_const_nxt = {
    {&pb_enum_type_AnalogType},
    .name = MP_QSTR_NXT,
    .value = PBIO_IODEV_TYPE_ID_NXT_ANALOG
};

STATIC const mp_rom_map_elem_t pb_enum_AnalogType_table[] = {
    { MP_ROM_QSTR(MP_QSTR_EV3),  MP_ROM_PTR(&pb_const_ev3) },
    { MP_ROM_QSTR(MP_QSTR_NXT),  MP_ROM_PTR(&pb_const_nxt) },
};
PB_DEFINE_ENUM(pb_enum_type_AnalogType, MP_QSTR_AnalogType, pb_enum_AnalogType_table);

// pybricks.ev3devices.AnalogSensor.__init__
STATIC mp_obj_t ev3devices_AnalogSensor_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_ENUM(type, pb_const_nxt),
        PB_ARG_DEFAULT_FALSE(force)
    );
    ev3devices_AnalogSensor_obj_t *self = m_new_obj(ev3devices_AnalogSensor_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
    mp_int_t type_arg = enum_get_value_maybe(type, &pb_enum_type_AnalogType);
    bool force_arg = mp_obj_is_true(force);

    if (force_arg) {
        // TODO: Force the port to user-specified analog
    }

    // Get the device
    pb_assert(ev3device_get_device(&self->iodev, type_arg, port_num));

    // Initialize NXT sensors to passive state
    if (self->iodev->type_id == PBIO_IODEV_TYPE_ID_NXT_ANALOG) {
        int32_t voltage;
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage));
    }

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.AnalogSensor.__str__
STATIC void ev3devices_AnalogSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_AnalogSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.ev3devices.AnalogSensor.voltage
STATIC mp_obj_t ev3devices_AnalogSensor_voltage(mp_obj_t self_in) {
    ev3devices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode;
    
    // EV3 Analog Sensors have only one mode
    if (self->iodev->type_id == PBIO_IODEV_TYPE_ID_EV3_ANALOG) {
        mode = PBIO_IODEV_MODE_EV3_ANALOG__RAW;
    }
    // NXT Analog Sensors can be passive or active (pin 5 state)
    else if (self->active) {
        mode = PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE;
    }
    else {
        mode = PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    }
    pb_assert(ev3device_get_values_at_mode(self->iodev, mode, &voltage));
    return mp_obj_new_int(voltage);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_AnalogSensor_voltage_obj, ev3devices_AnalogSensor_voltage);

// pybricks.ev3devices.AnalogSensor (internal)
STATIC mp_obj_t ev3devices_AnalogSensor_state(mp_obj_t self_in, bool active) {
    ev3devices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->iodev->type_id == PBIO_IODEV_TYPE_ID_NXT_ANALOG) {
        int32_t voltage;
        pb_assert(ev3device_get_values_at_mode(self->iodev,
                                               active ?
                                               PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE :
                                               PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE,
                                               &voltage));
        self->active = active;
        return mp_const_none;
    }
    pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    return mp_const_none;
}

// pybricks.ev3devices.AnalogSensor.active
STATIC mp_obj_t ev3devices_AnalogSensor_active(mp_obj_t self_in) {
    return ev3devices_AnalogSensor_state(self_in, true);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_AnalogSensor_active_obj, ev3devices_AnalogSensor_active);

// pybricks.ev3devices.AnalogSensor.passive
STATIC mp_obj_t ev3devices_AnalogSensor_passive(mp_obj_t self_in) {
    return ev3devices_AnalogSensor_state(self_in, false);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_AnalogSensor_passive_obj, ev3devices_AnalogSensor_passive);

// dir(pybricks.ev3devices.AnalogSensor)
STATIC const mp_rom_map_elem_t ev3devices_AnalogSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_Type),    MP_ROM_PTR(&pb_enum_type_AnalogType)             },
    { MP_ROM_QSTR(MP_QSTR_voltage), MP_ROM_PTR(&ev3devices_AnalogSensor_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_active),  MP_ROM_PTR(&ev3devices_AnalogSensor_active_obj)  },
    { MP_ROM_QSTR(MP_QSTR_passive), MP_ROM_PTR(&ev3devices_AnalogSensor_passive_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_AnalogSensor_locals_dict, ev3devices_AnalogSensor_locals_dict_table);

// type(pybricks.ev3devices.AnalogSensor)
STATIC const mp_obj_type_t ev3devices_AnalogSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_AnalogSensor,
    .print = ev3devices_AnalogSensor_print,
    .make_new = ev3devices_AnalogSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_AnalogSensor_locals_dict,
};

// dir(pybricks.ev3devices)
STATIC const mp_rom_map_elem_t ev3devices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_ev3devices)              },
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&motor_Motor_type)                },
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),      MP_ROM_PTR(&ev3devices_TouchSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor),   MP_ROM_PTR(&ev3devices_InfraredSensor_type)  },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),      MP_ROM_PTR(&ev3devices_ColorSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor), MP_ROM_PTR(&ev3devices_UltrasonicSensor_type)},
    { MP_ROM_QSTR(MP_QSTR_GyroSensor),       MP_ROM_PTR(&ev3devices_GyroSensor_type)      },
    { MP_ROM_QSTR(MP_QSTR_AnalogSensor),     MP_ROM_PTR(&ev3devices_AnalogSensor_type)    },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_ev3devices_globals, ev3devices_globals_table);
const mp_obj_module_t pb_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3devices_globals,
};
