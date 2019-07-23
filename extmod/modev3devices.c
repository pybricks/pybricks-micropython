// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include "modmotor.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "pbobj.h"
#include "pbkwarg.h"

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
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_TOUCH_SENSOR, mp_obj_get_int(port)));
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
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_IR_SENSOR, mp_obj_get_int(port)));
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
    { MP_ROM_QSTR(MP_QSTR_hellow), MP_OBJ_NEW_SMALL_INT(25) },
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
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    ev3devices_ColorSensor_obj_t *self = m_new_obj(ev3devices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR, mp_obj_get_int(args[0])));
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
        case 1: return mp_obj_new_int(PBIO_LIGHT_COLOR_BLACK);
        case 2: return mp_obj_new_int(PBIO_LIGHT_COLOR_BLUE);
        case 3: return mp_obj_new_int(PBIO_LIGHT_COLOR_GREEN);
        case 4: return mp_obj_new_int(PBIO_LIGHT_COLOR_YELLOW);
        case 5: return mp_obj_new_int(PBIO_LIGHT_COLOR_RED);
        case 6: return mp_obj_new_int(PBIO_LIGHT_COLOR_WHITE);
        case 7: return mp_obj_new_int(PBIO_LIGHT_COLOR_BROWN);
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

// pybricks.ev3devices.Light class object
typedef struct _ev3devices_Light_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
} ev3devices_Light_obj_t;

// pybricks.ev3devices.Light.blink
STATIC mp_obj_t ev3devices_Light_blink(mp_obj_t self_in) {
    ev3devices_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t unused;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__LISTEN, &unused));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_Light_blink_obj, ev3devices_Light_blink);

// pybricks.ev3devices.Light.on
STATIC mp_obj_t ev3devices_Light_on(mp_obj_t self_in) {
    ev3devices_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t unused;
    // TODO: Move to modlight and generalize to deal with any light instance
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM, &unused));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_Light_on_obj, ev3devices_Light_on);

// pybricks.ev3devices.Light.off
STATIC mp_obj_t ev3devices_Light_off(mp_obj_t self_in) {
    ev3devices_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t unused;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM, &unused));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_Light_off_obj, ev3devices_Light_off);

// dir(pybricks.ev3devices.Light)
STATIC const mp_rom_map_elem_t ev3devices_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_blink), MP_ROM_PTR(&ev3devices_Light_blink_obj) },
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&ev3devices_Light_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&ev3devices_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_Light_locals_dict, ev3devices_Light_locals_dict_table);

// type(pybricks.ev3devices.Light)
STATIC const mp_obj_type_t ev3devices_Light_type = {
    { &mp_type_type },
    .locals_dict = (mp_obj_dict_t*)&ev3devices_Light_locals_dict,
};

// pybricks.ev3devices.UltrasonicSensor class object
typedef struct _ev3devices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    pbio_ev3iodev_t *iodev;
} ev3devices_UltrasonicSensor_obj_t;

STATIC void pb_get_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    if (dest[0] == MP_OBJ_NULL) {

        // Return the light attribute
        if (attr == MP_QSTR_light) {
            ev3devices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
            dest[0] = self->light;
        }
        // For all other cases, do a generic lookup
        mp_obj_type_t *type = mp_obj_get_type(self_in);
        mp_map_t *locals_map = &type->locals_dict->map;
        mp_map_elem_t *elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
        if (elem != NULL) {
            mp_convert_member_lookup(self_in, type, elem->value, dest);
        }
    }
}

// pybricks.ev3devices.UltrasonicSensor.__init__
STATIC mp_obj_t ev3devices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    ev3devices_UltrasonicSensor_obj_t *self = m_new_obj(ev3devices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR, mp_obj_get_int(port)));

    // Create an instance of the Light class
    ev3devices_Light_obj_t *light = m_new_obj(ev3devices_Light_obj_t);
    light->base.type = (mp_obj_type_t*) &ev3devices_Light_type;
    light->iodev = self->iodev;
    self->light = light;

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.UltrasonicSensor.__str__
STATIC void ev3devices_UltrasonicSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_UltrasonicSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.ev3devices.UltrasonicSensor.distance
STATIC mp_obj_t ev3devices_UltrasonicSensor_distance(mp_obj_t self_in) {
    ev3devices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t distance;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM, &distance));
    return mp_obj_new_int(distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_UltrasonicSensor_distance_obj, ev3devices_UltrasonicSensor_distance);

// dir(pybricks.ev3devices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t ev3devices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&ev3devices_UltrasonicSensor_distance_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_UltrasonicSensor_locals_dict, ev3devices_UltrasonicSensor_locals_dict_table);

// type(pybricks.ev3devices.UltrasonicSensor)
STATIC const mp_obj_type_t ev3devices_UltrasonicSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_UltrasonicSensor,
    .print = ev3devices_UltrasonicSensor_print,
    .make_new = ev3devices_UltrasonicSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_UltrasonicSensor_locals_dict,
    .attr = pb_get_attr,
};

// dir(pybricks.ev3devices)
STATIC const mp_rom_map_elem_t ev3devices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_ev3devices)              },
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&motor_Motor_type)                },
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),      MP_ROM_PTR(&ev3devices_TouchSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor),   MP_ROM_PTR(&ev3devices_InfraredSensor_type)  },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),      MP_ROM_PTR(&ev3devices_ColorSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor), MP_ROM_PTR(&ev3devices_UltrasonicSensor_type)},
};

STATIC MP_DEFINE_CONST_DICT(pb_module_ev3devices_globals, ev3devices_globals_table);
const mp_obj_module_t pb_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3devices_globals,
};
