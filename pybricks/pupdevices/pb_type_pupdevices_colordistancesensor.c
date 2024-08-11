// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbio/button.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for ColorDistanceSensor. Note: first two members must match pb_ColorSensor_obj_t
typedef struct _pupdevices_ColorDistanceSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    mp_obj_t color_map;
    mp_obj_t light;
} pupdevices_ColorDistanceSensor_obj_t;

/**
 * Gets base powered up object from the sensor. Used for Power Functions motor.
 *
 * @param [in] obj        ColorDistanceSensor object.
 */
pb_type_device_obj_base_t *pupdevices_ColorDistanceSensor__get_device(mp_obj_t obj) {
    pb_assert_type(obj, &pb_type_pupdevices_ColorDistanceSensor);
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(obj);
    return &self->device_base;
}

/**
 * Callback for turning on the sensor light (red/green/blue/off), used by
 * external ColorLight class instance.
 *
 * @param [in] context    Sensor base object.
 * @param [in] hsv        Requested color, will be rounded to nearest color.
 */
static mp_obj_t pupdevices_ColorDistanceSensor_light_on(void *context, const pbio_color_hsv_t *hsv) {
    pb_type_device_obj_base_t *sensor = context;

    // Even though the mode takes a 0-10 value for color, only red, green and blue
    // actually turn on the light. So we just pick the closest of these 3 to the
    // requested color.
    int8_t color;
    if (hsv->s < 50 || hsv->v < 50) {
        color = 0; // off
    } else if (hsv->h >= PBIO_COLOR_HUE_YELLOW && hsv->h < PBIO_COLOR_HUE_CYAN) {
        color = 5; // green
    } else if (hsv->h >= PBIO_COLOR_HUE_CYAN && hsv->h < PBIO_COLOR_HUE_MAGENTA) {
        color = 3; // blue
    } else {
        color = 9; // red
    }

    return pb_type_device_set_data(sensor, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__COL_O, &color, sizeof(color));
}

// pybricks.pupdevices.ColorDistanceSensor.__init__
static mp_obj_t pupdevices_ColorDistanceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorDistanceSensor_obj_t *self = mp_obj_malloc(pupdevices_ColorDistanceSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_COLOR_DIST_SENSOR);

    // Create an instance of the Light class
    self->light = pb_type_ColorLight_external_obj_new(&self->device_base, pupdevices_ColorDistanceSensor_light_on);

    // Save default color settings
    pb_color_map_save_default(&self->color_map);

    return MP_OBJ_FROM_PTR(self);
}

// Ensures sensor is in RGB mode then converts the measured raw RGB value to HSV.
static void get_hsv_data(pupdevices_ColorDistanceSensor_obj_t *self, pbio_color_hsv_t *hsv) {
    int16_t *raw = pb_type_device_get_data(MP_OBJ_FROM_PTR(&self->device_base), PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I);

    // Max observed value is ~440 so we scale to get a range of 0..255.
    pbio_color_rgb_t rgb;
    rgb.r = 1187 * raw[0] / 2048;
    rgb.g = 1187 * raw[1] / 2048;
    rgb.b = 1187 * raw[2] / 2048;
    pb_color_map_rgb_to_hsv(&rgb, hsv);
}

// pybricks.pupdevices.ColorDistanceSensor.color
static mp_obj_t get_color(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_hsv_t hsv;
    get_hsv_data(self, &hsv);
    return pb_color_map_get_color(&self->color_map, &hsv);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_color_obj, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, get_color);

// pybricks.pupdevices.ColorDistanceSensor.distance
static mp_obj_t get_distance(mp_obj_t self_in) {
    int8_t *data = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX);
    return mp_obj_new_int(data[0] * 10);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_distance_obj, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX, get_distance);

// pybricks.pupdevices.ColorDistanceSensor.reflection
static mp_obj_t get_reflection(mp_obj_t self_in) {
    int16_t *rgb = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I);
    return mp_obj_new_int((rgb[0] + rgb[1] + rgb[2]) / 12);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_reflection_obj, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, get_reflection);

// pybricks.pupdevices.ColorDistanceSensor.ambient
static mp_obj_t get_ambient(mp_obj_t self_in) {
    int8_t *ambient = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI);
    return mp_obj_new_int(ambient[0]);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_ambient_obj, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI, get_ambient);

// pybricks.pupdevices.ColorDistanceSensor.hsv
static mp_obj_t get_hsv(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_Color_obj_t *color = pb_type_Color_new_empty();
    get_hsv_data(self, &color->hsv);
    return MP_OBJ_FROM_PTR(color);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_hsv_obj, PBDRV_LEGODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, get_hsv);

static const pb_attr_dict_entry_t pupdevices_ColorDistanceSensor_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, pupdevices_ColorDistanceSensor_obj_t, light),
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.pupdevices.ColorDistanceSensor)
static const mp_rom_map_elem_t pupdevices_ColorDistanceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&get_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&get_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&get_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&get_distance_obj)             },
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&get_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_detectable_colors),   MP_ROM_PTR(&pb_ColorSensor_detectable_colors_obj)                            },
};
static MP_DEFINE_CONST_DICT(pupdevices_ColorDistanceSensor_locals_dict, pupdevices_ColorDistanceSensor_locals_dict_table);

// type(pybricks.pupdevices.ColorDistanceSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_ColorDistanceSensor,
    MP_QSTR_ColorDistanceSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_ColorDistanceSensor_make_new,
    attr, pb_attribute_handler,
    protocol, pupdevices_ColorDistanceSensor_attr_dict,
    locals_dict, &pupdevices_ColorDistanceSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
