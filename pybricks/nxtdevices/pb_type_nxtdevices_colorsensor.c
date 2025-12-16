// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/mphal.h"

#include <pbdrv/clock.h>

#include <pbio/int_math.h>
#include <pbio/port_interface.h>
#include <pbio/util.h>
#include <pbio/port_dcm.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_pb/pb_color_map.h>

// pybricks.nxtdevices.ColorSensor class object
typedef struct _pb_type_nxtdevices_colorsensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
    mp_obj_t color_map;
} pb_type_nxtdevices_colorsensor_obj_t;

// pybricks.nxtdevices.ColorSensor.ambient
static mp_obj_t pb_type_nxtdevices_colorsensor_ambient(mp_obj_t self_in) {
    pb_type_nxtdevices_colorsensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_port_dcm_analog_rgba_t *rgba;
    pb_assert(pbio_port_get_analog_rgba(self->port, LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR, &rgba));
    return pb_obj_new_fraction(rgba->a, 10);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_nxtdevices_colorsensor_ambient_obj, pb_type_nxtdevices_colorsensor_ambient);

// pybricks.nxtdevices.ColorSensor.__init__
static mp_obj_t pb_type_nxtdevices_colorsensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pb_type_nxtdevices_colorsensor_obj_t *self = mp_obj_malloc(pb_type_nxtdevices_colorsensor_obj_t, type);
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(pbio_port_get_port(port_id, &self->port));

    pb_module_tools_assert_blocking();

    // On NXT, this activates the background process to drive this sensor.
    pb_assert(pbio_port_set_type(self->port, LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR));

    // Assert that the device is there.
    pbio_port_dcm_analog_rgba_t *rgba;
    pb_assert(pbio_port_get_analog_rgba(self->port, LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR, &rgba));

    // Wait for a recent sample. On EV3 with autodetection this never waits in
    // practice. On NXT we activate this sensor process manually the first time,
    // so we need to wait a little while to get a new sample.
    uint32_t start_time = pbdrv_clock_get_ms();
    while (!pbio_util_time_has_passed(rgba->last_sample_time, start_time - 100)) {
        pb_assert(pbio_port_get_analog_rgba(self->port, LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR, &rgba));
        mp_hal_delay_ms(10);
    }

    // Save default settings
    pb_color_map_save_default(&self->color_map);

    return MP_OBJ_FROM_PTR(self);
}

/**
 * Gets the RGB data from the sensor and converts it to HSV.
 *
 * @param  [in]  self  The sensor object.
 * @param  [out] hsv   The HSV data.
 */
static void get_hsv_data(pb_type_nxtdevices_colorsensor_obj_t *self, pbio_color_hsv_t *hsv) {
    pbio_port_dcm_analog_rgba_t *rgba;
    pb_assert(pbio_port_get_analog_rgba(self->port, LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR, &rgba));

    // Values are capped between 0--1000, so scale to get a range of 0..255.
    pbio_color_rgb_t rgb;
    rgb.r = rgba->r >> 2;
    rgb.g = rgba->g >> 2;
    rgb.b = rgba->b >> 2;
    pb_color_map_rgb_to_hsv(&rgb, hsv);
}

// pybricks.nxtdevices.ColorSensor.color
static mp_obj_t pb_type_nxtdevices_colorsensor_color(mp_obj_t self_in) {
    pb_type_nxtdevices_colorsensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_color_hsv_t hsv;
    get_hsv_data(self, &hsv);
    return pb_color_map_get_color(&self->color_map, &hsv);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_nxtdevices_colorsensor_color_obj, pb_type_nxtdevices_colorsensor_color);

// pybricks.nxtdevices.ColorSensor.hsv
static mp_obj_t pb_type_nxtdevices_colorsensor_hsv(mp_obj_t self_in) {
    pb_type_nxtdevices_colorsensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_Color_obj_t *color = pb_type_Color_new_empty();
    get_hsv_data(self, &color->hsv);
    return MP_OBJ_FROM_PTR(color);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_nxtdevices_colorsensor_hsv_obj, pb_type_nxtdevices_colorsensor_hsv);

// pybricks.nxtdevices.ColorSensor.reflection
static mp_obj_t pb_type_nxtdevices_colorsensor_reflection(mp_obj_t self_in) {
    pb_type_nxtdevices_colorsensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_port_dcm_analog_rgba_t *rgba;
    pb_assert(pbio_port_get_analog_rgba(self->port, LEGO_DEVICE_TYPE_ID_NXT_COLOR_SENSOR, &rgba));
    return pb_obj_new_fraction((rgba->r + rgba->g + rgba->b) / 3, 10);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_nxtdevices_colorsensor_reflection_obj, pb_type_nxtdevices_colorsensor_reflection);

// pybricks.nxtdevices.ColorSensor.detectable_colors
static mp_obj_t detectable_colors(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_nxtdevices_colorsensor_obj_t, self,
        PB_ARG_DEFAULT_NONE(colors));
    return pb_color_map_detectable_colors_method(&self->color_map, colors_in);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(detectable_colors_obj, 1, detectable_colors);

// dir(pybricks.nxtdevices.ColorSensor)
static const mp_rom_map_elem_t pb_type_nxtdevices_colorsensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ambient),           MP_ROM_PTR(&pb_type_nxtdevices_colorsensor_ambient_obj)    },
    { MP_ROM_QSTR(MP_QSTR_color),             MP_ROM_PTR(&pb_type_nxtdevices_colorsensor_color_obj)      },
    { MP_ROM_QSTR(MP_QSTR_hsv),               MP_ROM_PTR(&pb_type_nxtdevices_colorsensor_hsv_obj)        },
    { MP_ROM_QSTR(MP_QSTR_reflection),        MP_ROM_PTR(&pb_type_nxtdevices_colorsensor_reflection_obj) },
    { MP_ROM_QSTR(MP_QSTR_detectable_colors), MP_ROM_PTR(&detectable_colors_obj)                         },
};
static MP_DEFINE_CONST_DICT(pb_type_nxtdevices_colorsensor_locals_dict, pb_type_nxtdevices_colorsensor_locals_dict_table);

// type(pybricks.nxtdevices.ColorSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_ColorSensor,
    MP_QSTR_ColorSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_nxtdevices_colorsensor_make_new,
    locals_dict, &pb_type_nxtdevices_colorsensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
