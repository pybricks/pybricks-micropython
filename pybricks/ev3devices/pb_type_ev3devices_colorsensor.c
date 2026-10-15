// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES


#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/ev3devices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for ColorSensor
typedef struct _ev3devices_ColorSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    pbio_color_map_t *color_map;
} ev3devices_ColorSensor_obj_t;

// pybricks.ev3devices.ColorSensor.__init__
static mp_obj_t ev3devices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    ev3devices_ColorSensor_obj_t *self = pb_type_device_make_new(type, n_args, n_kw, args,
        sizeof(ev3devices_ColorSensor_obj_t), LEGO_DEVICE_TYPE_ID_EV3_COLOR_SENSOR);

    // Save default color settings
    self->color_map = pb_color_map_init(self->device_base.port);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.ColorSensor.color
static mp_obj_t get_color(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_t matched;
    pb_assert(pbio_port_get_color(self->device_base.port, NULL, &matched, NULL, NULL));
    return pb_color_map_get_color(self->color_map, matched);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_color_obj, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW, get_color);

// pybricks.ev3devices.ColorSensor.hsv
static mp_obj_t get_hsv(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_t hsv;
    pb_assert(pbio_port_get_color(self->device_base.port, &hsv, NULL, NULL, NULL));
    return pb_type_Color_new(hsv);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_hsv_obj, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW, get_hsv);

// pybricks.ev3devices.ColorSensor.ambient
static mp_obj_t get_ambient(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t intensity;
    pb_assert(pbio_port_get_light_intensity(self->device_base.port, NULL, &intensity, NULL, NULL));
    return pb_obj_new_fraction(intensity, 10);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_ambient_obj, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__AMBIENT, get_ambient);

// pybricks.ev3devices.ColorSensor.reflection
static mp_obj_t get_reflection(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t intensity;
    pb_assert(pbio_port_get_light_intensity(self->device_base.port, &intensity, NULL, NULL, NULL));
    return pb_obj_new_fraction(intensity, 10);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_reflection_obj, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW, get_reflection);

// pybricks.ev3devices.ColorSensor.rgb
static mp_obj_t get_rgb(mp_obj_t self_in) {
    int16_t *rgb = pb_type_device_get_data(self_in, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW);
    mp_obj_t tup[3];
    for (uint8_t i = 0; i < 3; i++) {
        tup[i] = mp_obj_new_int(rgb[i]);
    }
    return mp_obj_new_tuple(3, tup);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_rgb_obj, LEGO_DEVICE_MODE_EV3_COLOR_SENSOR__RGB_RAW, get_rgb);

// pybricks.ev3devices.ColorSensor.detectable_colors
static mp_obj_t detectable_colors(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_NONE(colors));
    return pb_color_map_detectable_colors_method(self->color_map, colors_in);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(detectable_colors_obj, 1, detectable_colors);

// dir(pybricks.ev3devices.ColorSensor)
static const mp_rom_map_elem_t ev3devices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reflection),        MP_ROM_PTR(&get_reflection_obj)    },
    { MP_ROM_QSTR(MP_QSTR_ambient),           MP_ROM_PTR(&get_ambient_obj)       },
    { MP_ROM_QSTR(MP_QSTR_color),             MP_ROM_PTR(&get_color_obj)         },
    { MP_ROM_QSTR(MP_QSTR_hsv),               MP_ROM_PTR(&get_hsv_obj)           },
    { MP_ROM_QSTR(MP_QSTR_rgb),               MP_ROM_PTR(&get_rgb_obj)           },
    { MP_ROM_QSTR(MP_QSTR_detectable_colors), MP_ROM_PTR(&detectable_colors_obj) },
};
static MP_DEFINE_CONST_DICT(ev3devices_ColorSensor_locals_dict, ev3devices_ColorSensor_locals_dict_table);

// type(pybricks.ev3devices.ColorSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_ev3devices_ColorSensor,
    MP_QSTR_ColorSensor,
    MP_TYPE_FLAG_NONE,
    make_new, ev3devices_ColorSensor_make_new,
    locals_dict, &ev3devices_ColorSensor_locals_dict);

#endif // PYBRICKS_PY_EV3DEVICES
