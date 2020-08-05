// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/mphal.h"

#include <pbdrv/ioport.h>

#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/iodev.h>
#include <pbio/light.h>
#include <pbio/math.h>

#include "py/obj.h"
#include "py/runtime.h"


#include "common/common.h"
#include "common/common_motors.h"
#include "parameters/parameters.h"

#include "util_pb/pb_color_map.h"
#include "util_pb/pb_device.h"
#include "util_pb/pb_error.h"
#include "util_mp/pb_kwarg_helper.h"
#include "util_mp/pb_obj_helper.h"

// Class structure for ColorDistanceSensor. Note: first two members must match pb_ColorSensor_obj_t
typedef struct _pupdevices_ColorDistanceSensor_obj_t {
    mp_obj_base_t base;
    pb_hsv_map_t color_map;
    pb_device_t *pbdev;
    mp_obj_t light;
} pupdevices_ColorDistanceSensor_obj_t;

STATIC void raw_to_rgb(int32_t *raw, pbio_color_rgb_t *rgb) {
    // Max observed value is ~440 so we scale to get a range of 0..255.
    rgb->r = 1187 * raw[0] / 2048;
    rgb->g = 1187 * raw[1] / 2048;
    rgb->b = 1187 * raw[2] / 2048;
}

// Ensures sensor is in RGB mode then converts the measured raw RGB value to HSV.
STATIC void pupdevices_ColorDistanceSensor__hsv(pupdevices_ColorDistanceSensor_obj_t *self, pbio_color_hsv_t *hsv) {
    int32_t raw[3];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, raw);

    pbio_color_rgb_t rgb;
    raw_to_rgb(raw, &rgb);

    pbio_color_rgb_to_hsv(&rgb, hsv);
}

// pybricks.pupdevices.ColorDistanceSensor.__init__
STATIC mp_obj_t pupdevices_ColorDistanceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorDistanceSensor_obj_t *self = m_new_obj(pupdevices_ColorDistanceSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);

    // Create an instance of the Light class
    self->light = common_ColorLight_obj_make_new(self->pbdev);

    // Save default color settings
    pb_hsv_map_save_default(&self->color_map);
    self->color_map.saturation_threshold = 50;

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ColorDistanceSensor.color
STATIC mp_obj_t pupdevices_ColorDistanceSensor_color(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_color_hsv_t hsv;
    pupdevices_ColorDistanceSensor__hsv(self, &hsv);

    // Get and return discretized color based on HSV
    return pb_hsv_get_color(&self->color_map, hsv.h, hsv.s, hsv.v);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_color_obj, pupdevices_ColorDistanceSensor_color);

// pybricks.pupdevices.ColorDistanceSensor.distance
STATIC mp_obj_t pupdevices_ColorDistanceSensor_distance(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t distance;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX, &distance);
    return mp_obj_new_int(distance * 10);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_distance_obj, pupdevices_ColorDistanceSensor_distance);

// pybricks.pupdevices.ColorDistanceSensor.reflection
STATIC mp_obj_t pupdevices_ColorDistanceSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t rgb[3];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, rgb);
    return mp_obj_new_int((rgb[0] + rgb[1] + rgb[2]) / 12);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_reflection_obj, pupdevices_ColorDistanceSensor_reflection);

// pybricks.pupdevices.ColorDistanceSensor.ambient
STATIC mp_obj_t pupdevices_ColorDistanceSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t ambient;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI, &ambient);
    return mp_obj_new_int(ambient);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_ambient_obj, pupdevices_ColorDistanceSensor_ambient);

// pybricks.pupdevices.ColorDistanceSensor.remote
STATIC mp_obj_t pupdevices_ColorDistanceSensor_remote(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorDistanceSensor_obj_t, self,
        PB_ARG_REQUIRED(channel),
        PB_ARG_DEFAULT_NONE(button_1),
        PB_ARG_DEFAULT_NONE(button_2));

    // Get channel
    mp_int_t ch = mp_obj_get_int(channel);
    if (ch < 1 || ch > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get individual button codes
    pbio_button_flags_t b1, b2, btn;
    b1 = button_1 == mp_const_none ? 0 : pb_type_enum_get_value(button_1, &pb_enum_type_Button);
    b2 = button_2 == mp_const_none ? 0 : pb_type_enum_get_value(button_2, &pb_enum_type_Button);

    // Full button mask
    btn = b1 | b2;

    // Power Functions 1.0 "Combo Direct Mode" without checksum
    int32_t message = ((btn & PBIO_BUTTON_LEFT_UP) != 0) << 0 |
                ((btn & PBIO_BUTTON_LEFT_DOWN) != 0) << 1 |
                ((btn & PBIO_BUTTON_RIGHT_UP) != 0) << 2 |
                ((btn & PBIO_BUTTON_RIGHT_DOWN) != 0) << 3 |
                (1) << 4 |
                (ch - 1) << 8;

    // Send the data to the device
    pb_device_set_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX, &message, 1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorDistanceSensor_remote_obj, 1, pupdevices_ColorDistanceSensor_remote);

// pybricks.pupdevices.ColorDistanceSensor.hsv
STATIC mp_obj_t pupdevices_ColorDistanceSensor_hsv(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_color_hsv_t hsv;
    pupdevices_ColorDistanceSensor__hsv(self, &hsv);

    mp_obj_t ret[3];
    ret[0] = mp_obj_new_int(hsv.h);
    ret[1] = mp_obj_new_int(hsv.s);
    ret[2] = mp_obj_new_int(hsv.v);
    return mp_obj_new_tuple(MP_ARRAY_SIZE(ret), ret);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_hsv_obj, pupdevices_ColorDistanceSensor_hsv);

// dir(pybricks.pupdevices.ColorDistanceSensor)
STATIC const mp_rom_map_elem_t pupdevices_ColorDistanceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&pupdevices_ColorDistanceSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_ColorDistanceSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&pupdevices_ColorDistanceSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_ColorDistanceSensor_distance_obj)             },
    { MP_ROM_QSTR(MP_QSTR_remote),      MP_ROM_PTR(&pupdevices_ColorDistanceSensor_remote_obj)               },
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&pupdevices_ColorDistanceSensor_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_color_map),   MP_ROM_PTR(&pb_ColorSensor_color_map_obj)                            },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(pupdevices_ColorDistanceSensor_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorDistanceSensor_locals_dict, pupdevices_ColorDistanceSensor_locals_dict_table);

// type(pybricks.pupdevices.ColorDistanceSensor)
const mp_obj_type_t pb_type_pupdevices_ColorDistanceSensor = {
    { &mp_type_type },
    .name = MP_QSTR_ColorDistanceSensor,
    .make_new = pupdevices_ColorDistanceSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_ColorDistanceSensor_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
