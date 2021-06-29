// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbio/button.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for ColorDistanceSensor. Note: first two members must match pb_ColorSensor_obj_t
typedef struct _pupdevices_ColorDistanceSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t color_map;
    pb_device_t *pbdev;
    mp_obj_t light;
} pupdevices_ColorDistanceSensor_obj_t;

STATIC void raw_to_rgb(int32_t *raw, pbio_color_rgb_t *rgb) {
    // Max observed value is ~440 so we scale to get a range of 0..255.
    rgb->r = 1187 * raw[0] / 2048;
    rgb->g = 1187 * raw[1] / 2048;
    rgb->b = 1187 * raw[2] / 2048;
}

pb_device_t *pupdevices_ColorDistanceSensor__get_device(mp_obj_t obj) {

    // Assert that this is a ColorDistanceSensor
    pb_assert_type(obj, &pb_type_pupdevices_ColorDistanceSensor);

    // Get and return device pointer
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(obj);
    return self->pbdev;
}

// Ensures sensor is in RGB mode then converts the measured raw RGB value to HSV.
STATIC void pupdevices_ColorDistanceSensor__hsv(pupdevices_ColorDistanceSensor_obj_t *self, pbio_color_hsv_t *hsv) {
    int32_t raw[3];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, raw);

    pbio_color_rgb_t rgb;
    raw_to_rgb(raw, &rgb);

    color_map_rgb_to_hsv(&rgb, hsv);
}

// pybricks.pupdevices.ColorDistanceSensor.__init__
STATIC mp_obj_t pupdevices_ColorDistanceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorDistanceSensor_obj_t *self = m_new_obj(pupdevices_ColorDistanceSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);

    // Create an instance of the Light class
    self->light = common_ColorLight_external_obj_make_new(self->pbdev);

    // Save default color settings
    pb_color_map_save_default(&self->color_map);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ColorDistanceSensor.color
STATIC mp_obj_t pupdevices_ColorDistanceSensor_color(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_color_hsv_t hsv;
    pupdevices_ColorDistanceSensor__hsv(self, &hsv);

    // Get and return discretized color based on HSV
    return pb_color_map_get_color(&self->color_map, &hsv);
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

// pybricks.pupdevices.ColorDistanceSensor.hsv
STATIC mp_obj_t pupdevices_ColorDistanceSensor_hsv(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Create color object
    pb_type_Color_obj_t *color = pb_type_Color_new_empty();

    // Read HSV
    pupdevices_ColorDistanceSensor__hsv(self, &color->hsv);

    // Return color
    return MP_OBJ_FROM_PTR(color);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_hsv_obj, pupdevices_ColorDistanceSensor_hsv);

// dir(pybricks.pupdevices.ColorDistanceSensor)
STATIC const mp_rom_map_elem_t pupdevices_ColorDistanceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&pupdevices_ColorDistanceSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_ColorDistanceSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&pupdevices_ColorDistanceSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_ColorDistanceSensor_distance_obj)             },
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&pupdevices_ColorDistanceSensor_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_detectable_colors),   MP_ROM_PTR(&pb_ColorSensor_detectable_colors_obj)                            },
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
