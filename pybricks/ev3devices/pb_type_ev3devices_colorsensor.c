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

// pybricks.ev3devices.ColorSensor class object
typedef struct _ev3devices_ColorSensor_obj_t {
    pb_type_device_obj_base_t device_base;
} ev3devices_ColorSensor_obj_t;

// pybricks.ev3devices.ColorSensor.__init__
static mp_obj_t ev3devices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));
    ev3devices_ColorSensor_obj_t *self = mp_obj_malloc(ev3devices_ColorSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_EV3_COLOR_SENSOR);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.ColorSensor.color
static mp_obj_t get_color(mp_obj_t self_in) {
    int8_t *color = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__COLOR);
    switch (color[0]) {
        case 1:
            return MP_OBJ_FROM_PTR(&pb_Color_BLACK_obj);
        case 2:
            return MP_OBJ_FROM_PTR(&pb_Color_BLUE_obj);
        case 3:
            return MP_OBJ_FROM_PTR(&pb_Color_GREEN_obj);
        case 4:
            return MP_OBJ_FROM_PTR(&pb_Color_YELLOW_obj);
        case 5:
            return MP_OBJ_FROM_PTR(&pb_Color_RED_obj);
        case 6:
            return MP_OBJ_FROM_PTR(&pb_Color_WHITE_obj);
        case 7:
            return MP_OBJ_FROM_PTR(&pb_Color_BROWN_obj);
        default:
            return mp_const_none;
    }
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_color_obj, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__COLOR, get_color);

// pybricks.ev3devices.ColorSensor.ambient
static mp_obj_t get_ambient(mp_obj_t self_in) {
    int8_t *ambient = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__AMBIENT);
    return mp_obj_new_int(ambient[0]);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_ambient_obj, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__AMBIENT, get_ambient);

// pybricks.ev3devices.ColorSensor.reflection
static mp_obj_t get_reflection(mp_obj_t self_in) {
    int8_t *reflection = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__REFLECT);
    return mp_obj_new_int(reflection[0]);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_reflection_obj, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__REFLECT, get_reflection);

// pybricks.ev3devices.ColorSensor.rgb
static mp_obj_t get_rgb(mp_obj_t self_in) {
    int16_t *rgb = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__RGB_RAW);
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
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_rgb_obj, PBDRV_LEGODEV_MODE_EV3_COLOR_SENSOR__RGB_RAW, get_rgb);

// dir(pybricks.ev3devices.ColorSensor)
static const mp_rom_map_elem_t ev3devices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&get_reflection_obj) },
    { MP_ROM_QSTR(MP_QSTR_ambient), MP_ROM_PTR(&get_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_color), MP_ROM_PTR(&get_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_rgb), MP_ROM_PTR(&get_rgb_obj) },
};
static MP_DEFINE_CONST_DICT(ev3devices_ColorSensor_locals_dict, ev3devices_ColorSensor_locals_dict_table);

// type(pybricks.ev3devices.ColorSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_ev3devices_ColorSensor,
    MP_QSTR_ColorSensor,
    MP_TYPE_FLAG_NONE,
    make_new, ev3devices_ColorSensor_make_new,
    locals_dict, &ev3devices_ColorSensor_locals_dict);

#endif // PYBRICKS_PY_EV3DEVICES
