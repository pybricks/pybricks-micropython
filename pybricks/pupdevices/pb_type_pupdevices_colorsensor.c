// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for ColorSensor. Note: first two members must match pb_ColorSensor_obj_t
typedef struct _pupdevices_ColorSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t color_map;
    pb_device_t *pbdev;
    mp_obj_t lights;
} pupdevices_ColorSensor_obj_t;

// pybricks._common.ColorSensor._get_hsv
STATIC void pupdevices_ColorSensor__get_hsv(pb_device_t *pbdev, bool light_on, pbio_color_hsv_t *hsv) {

    // Read HSV (light on) or SHSV mode (light off)
    int32_t data[4];
    pb_device_get_values(pbdev, light_on ? PBIO_IODEV_MODE_PUP_COLOR_SENSOR__HSV : PBIO_IODEV_MODE_PUP_COLOR_SENSOR__SHSV, data);

    // Scale saturation and value to match 0-100% range in typical applications.
    // However, do not cap to allow other applications as well. For example, full sunlight will exceed 100%.
    hsv->h = data[0];
    if (light_on) {
        hsv->s = data[1] / 8;
        hsv->v = data[2] / 5;
    } else {
        hsv->s = data[1] / 12;
        hsv->v = data[2] / 12;
    }
}

// pybricks.pupdevices.ColorSensor.__init__
STATIC mp_obj_t pupdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorSensor_obj_t *self = m_new_obj(pupdevices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_SPIKE_COLOR_SENSOR);

    // This sensor requires power, which iodevice does not do automatically yet
    pb_device_set_power_supply(self->pbdev, 100);

    // Create an instance of the LightArray class
    self->lights = common_LightArray_obj_make_new(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_SENSOR__LIGHT, 3);

    // Do one reading to make sure everything is working and to set default mode
    pbio_color_hsv_t hsv;
    pupdevices_ColorSensor__get_hsv(self->pbdev, true, &hsv);

    // Save default settings
    pb_color_map_save_default(&self->color_map);

    // This sensor needs some time to get values right after turning power on
    mp_hal_delay_ms(1000);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks._common.ColorSensor.hsv
STATIC mp_obj_t pupdevices_ColorSensor_hsv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    // Create color object
    pb_type_Color_obj_t *color = pb_type_Color_new_empty();

    // Read HSV, either with light on or off
    pupdevices_ColorSensor__get_hsv(self->pbdev, mp_obj_is_true(surface_in), &color->hsv);

    // Return color
    return MP_OBJ_FROM_PTR(color);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorSensor_hsv_obj, 1, pupdevices_ColorSensor_hsv);

// pybricks._common.ColorSensor.color
STATIC mp_obj_t pupdevices_ColorSensor_color(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    // Read HSV, either with light on or off
    pbio_color_hsv_t hsv;
    pupdevices_ColorSensor__get_hsv(self->pbdev, mp_obj_is_true(surface_in), &hsv);

    // Get and return discretized color
    return pb_color_map_get_color(&self->color_map, &hsv);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorSensor_color_obj, 1, pupdevices_ColorSensor_color);

// pybricks.pupdevices.ColorSensor.reflection
STATIC mp_obj_t pupdevices_ColorSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Read HSV with light on
    pbio_color_hsv_t hsv;
    pupdevices_ColorSensor__get_hsv(self->pbdev, true, &hsv);

    // Return value as reflection
    return mp_obj_new_int(hsv.v);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorSensor_reflection_obj, pupdevices_ColorSensor_reflection);

// pybricks.pupdevices.ColorSensor.ambient
STATIC mp_obj_t pupdevices_ColorSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Read HSV with light off
    pbio_color_hsv_t hsv;
    pupdevices_ColorSensor__get_hsv(self->pbdev, false, &hsv);

    // Return value as ambient intensity
    return mp_obj_new_int(hsv.v);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorSensor_ambient_obj, pupdevices_ColorSensor_ambient);

// dir(pybricks.pupdevices.ColorSensor)
STATIC const mp_rom_map_elem_t pupdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_lights),      MP_ROM_ATTRIBUTE_OFFSET(pupdevices_ColorSensor_obj_t, lights)},
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&pupdevices_ColorSensor_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&pupdevices_ColorSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_ColorSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&pupdevices_ColorSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_color_map),   MP_ROM_PTR(&pb_ColorSensor_color_map_obj)                    },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorSensor_locals_dict, pupdevices_ColorSensor_locals_dict_table);

// type(pybricks.pupdevices.ColorSensor)
const mp_obj_type_t pb_type_pupdevices_ColorSensor = {
    { &mp_type_type },
    .name = MP_QSTR_ColorSensor,
    .make_new = pupdevices_ColorSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_ColorSensor_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
