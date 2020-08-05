// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include "py/objtype.h"

#include <pbio/iodev.h>
#include <pbio/button.h>

#include <pybricks/common.h>
#include <pybricks/ev3devices.h>

#include <pybricks/parameters.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.ev3devices.ColorSensor class object
typedef struct _ev3devices_ColorSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} ev3devices_ColorSensor_obj_t;

// pybricks.ev3devices.ColorSensor.__init__
STATIC mp_obj_t ev3devices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));
    ev3devices_ColorSensor_obj_t *self = m_new_obj(ev3devices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.ColorSensor.color
STATIC mp_obj_t ev3devices_ColorSensor_color(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t color;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__COLOR, &color);

    switch (color) {
        case 1:
            return pb_const_color_black;
        case 2:
            return pb_const_color_blue;
        case 3:
            return pb_const_color_green;
        case 4:
            return pb_const_color_yellow;
        case 5:
            return pb_const_color_red;
        case 6:
            return pb_const_color_white;
        case 7:
            return pb_const_color_brown;
        default:
            return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_color_obj, ev3devices_ColorSensor_color);

// pybricks.ev3devices.ColorSensor.ambient
STATIC mp_obj_t ev3devices_ColorSensor_ambient(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t ambient;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__AMBIENT, &ambient);
    return mp_obj_new_int(ambient);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_ambient_obj, ev3devices_ColorSensor_ambient);

// pybricks.ev3devices.ColorSensor.reflection
STATIC mp_obj_t ev3devices_ColorSensor_reflection(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t reflection;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__REFLECT, &reflection);
    return mp_obj_new_int(reflection);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_reflection_obj, ev3devices_ColorSensor_reflection);

// pybricks.ev3devices.ColorSensor.rgb
STATIC mp_obj_t ev3devices_ColorSensor_rgb(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t rgb[3];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_COLOR_SENSOR__RGB_RAW, rgb);
    mp_obj_t tup[3];

    rgb[0] = (int)((0.258 * rgb[0]) - 0.3);
    rgb[1] = (int)((0.280 * rgb[1]) - 0.8);
    rgb[2] = (int)((0.523 * rgb[2]) - 3.7);

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
    { MP_ROM_QSTR(MP_QSTR_ambient), MP_ROM_PTR(&ev3devices_ColorSensor_ambient_obj)    },
    { MP_ROM_QSTR(MP_QSTR_color), MP_ROM_PTR(&ev3devices_ColorSensor_color_obj)      },
    { MP_ROM_QSTR(MP_QSTR_rgb), MP_ROM_PTR(&ev3devices_ColorSensor_rgb_obj)        },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_ColorSensor_locals_dict, ev3devices_ColorSensor_locals_dict_table);

// type(pybricks.ev3devices.ColorSensor)
const mp_obj_type_t pb_type_ev3devices_ColorSensor = {
    { &mp_type_type },
    .name = MP_QSTR_ColorSensor,
    .make_new = ev3devices_ColorSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&ev3devices_ColorSensor_locals_dict,
};

#endif // PYBRICKS_PY_EV3DEVICES
