// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for ColorSensor. Note: first two members must match pb_ColorSensor_obj_t
typedef struct _pupdevices_ColorSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    pbio_color_map_t *color_map;
    mp_obj_t lights;
} pupdevices_ColorSensor_obj_t;

// pybricks.pupdevices.ColorSensor.__init__
static mp_obj_t pupdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    pupdevices_ColorSensor_obj_t *self = pb_type_device_make_new(type, n_args, n_kw, args,
        sizeof(pupdevices_ColorSensor_obj_t), LEGO_DEVICE_TYPE_ID_SPIKE_COLOR_SENSOR);

    // Create an instance of the LightArray class
    self->lights = common_LightArray_obj_make_new(&self->device_base, LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__LIGHT, 3);

    // Do one reading to make sure everything is working and to set default mode
    pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I);

    // Save default settings
    self->color_map = pb_color_map_init(self->device_base.port);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ColorSensor.reflection
static mp_obj_t get_reflection(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t intensity;
    pb_assert(pbio_port_get_light_intensity(self->device_base.port, &intensity, true));
    return pb_obj_new_fraction(intensity, 10);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_reflection_obj, LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I, get_reflection);

// pybricks.pupdevices.ColorSensor.ambient
static mp_obj_t get_ambient(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t intensity;
    pb_assert(pbio_port_get_light_intensity(self->device_base.port, &intensity, false));
    return pb_obj_new_fraction(intensity, 10);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_ambient_obj, LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__SHSV, get_ambient);

// pybricks.pupdevices.ColorSensor.hsv(surface=True)
static mp_obj_t get_hsv_surface_true(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_t hsv;
    pb_assert(pbio_port_get_color(self->device_base.port, &hsv, NULL, true));
    return pb_type_Color_new(hsv);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_hsv_surface_true_obj, LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I, get_hsv_surface_true);

// pybricks.pupdevices.ColorSensor.hsv(surface=False)
static mp_obj_t get_hsv_surface_false(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_t hsv;
    pb_assert(pbio_port_get_color(self->device_base.port, &hsv, NULL, false));
    return pb_type_Color_new(hsv);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_hsv_surface_false_obj, LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__SHSV, get_hsv_surface_false);

// pybricks.pupdevices.ColorSensor.color(surface=True)
static mp_obj_t get_color_surface_true(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_t matched;
    pb_assert(pbio_port_get_color(self->device_base.port, NULL, &matched, true));
    return pb_color_map_get_color(self->color_map, matched);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_color_surface_true_obj, LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__RGB_I, get_color_surface_true);

// pybricks.pupdevices.ColorSensor.color(surface=False)
static mp_obj_t get_color_surface_false(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_t matched;
    pb_assert(pbio_port_get_color(self->device_base.port, NULL, &matched, false));
    return pb_color_map_get_color(self->color_map, matched);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_color_surface_false_obj, LEGO_DEVICE_MODE_PUP_COLOR_SENSOR__SHSV, get_color_surface_false);

// pybricks.pupdevices.ColorSensor.hsv
static mp_obj_t get_hsv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    (void)self;
    if (mp_obj_is_true(surface_in)) {
        return pb_type_device_method_call(MP_OBJ_FROM_PTR(&get_hsv_surface_true_obj), 1, 0, pos_args);
    }
    return pb_type_device_method_call(MP_OBJ_FROM_PTR(&get_hsv_surface_false_obj), 1, 0, pos_args);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(get_hsv_obj, 1, get_hsv);

// pybricks.pupdevices.ColorSensor.color
static mp_obj_t get_color(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    (void)self;
    if (mp_obj_is_true(surface_in)) {
        return pb_type_device_method_call(MP_OBJ_FROM_PTR(&get_color_surface_true_obj), 1, 0, pos_args);
    }
    return pb_type_device_method_call(MP_OBJ_FROM_PTR(&get_color_surface_false_obj), 1, 0, pos_args);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(get_color_obj, 1, get_color);

// pybricks.pupdevices.ColorSensor.detectable_colors
static mp_obj_t detectable_colors(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_NONE(colors));
    return pb_color_map_detectable_colors_method(self->color_map, colors_in);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(detectable_colors_obj, 1, detectable_colors);

static const pb_attr_dict_entry_t pupdevices_ColorSensor_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_lights, pupdevices_ColorSensor_obj_t, lights),
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.pupdevices.ColorSensor)
static const mp_rom_map_elem_t pupdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&get_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&get_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&get_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&get_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_detectable_colors),   MP_ROM_PTR(&detectable_colors_obj)},
};
static MP_DEFINE_CONST_DICT(pupdevices_ColorSensor_locals_dict, pupdevices_ColorSensor_locals_dict_table);

// type(pybricks.pupdevices.ColorSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_ColorSensor,
    MP_QSTR_ColorSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_ColorSensor_make_new,
    attr, pb_attribute_handler,
    protocol, pupdevices_ColorSensor_attr_dict,
    locals_dict, &pupdevices_ColorSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
