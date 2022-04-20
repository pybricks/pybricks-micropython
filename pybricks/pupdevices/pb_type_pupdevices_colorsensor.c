// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

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
    int32_t chroma_weight;
    pb_device_t *pbdev;
    mp_obj_t lights;
} pupdevices_ColorSensor_obj_t;

// pybricks._common.ColorSensor._get_hsv_reflected
STATIC void pupdevices_ColorSensor__get_hsv_reflected(pb_device_t *pbdev, pbio_color_hsv_t *hsv) {

    // Read RGB
    int32_t data[4];
    pb_device_get_values(pbdev, PBIO_IODEV_MODE_PUP_COLOR_SENSOR__RGB_I, data);
    const pbio_color_rgb_t rgb = {
        .r = data[0] == 1024 ? 255 : data[0] >> 2,
        .g = data[1] == 1024 ? 255 : data[1] >> 2,
        .b = data[2] == 1024 ? 255 : data[2] >> 2,
    };

    // Convert to HSV
    pb_color_map_rgb_to_hsv(&rgb, hsv);
}

// pybricks._common.ColorSensor._get_hsv_ambient
STATIC void pupdevices_ColorSensor__get_hsv_ambient(pb_device_t *pbdev, pbio_color_hsv_t *hsv) {

    // Read SHSV mode (light off). This data is not available in RGB format
    int32_t data[4];
    pb_device_get_values(pbdev, PBIO_IODEV_MODE_PUP_COLOR_SENSOR__SHSV, data);

    // Scale saturation and value to match 0-100% range in typical applications
    hsv->h = data[0];
    hsv->s = data[1] / 10;
    if (hsv->s > 100) {
        hsv->s = 100;
    }
    hsv->v = data[2] / 10;
    if (hsv->v > 100) {
        hsv->v = 100;
    }
}

// pybricks.pupdevices.ColorSensor.__init__
STATIC mp_obj_t pupdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorSensor_obj_t *self = m_new_obj(pupdevices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_SPIKE_COLOR_SENSOR);

    // Create an instance of the LightArray class
    self->lights = common_LightArray_obj_make_new(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_SENSOR__LIGHT, 3);

    // Do one reading to make sure everything is working and to set default mode
    pbio_color_hsv_t hsv;
    pupdevices_ColorSensor__get_hsv_reflected(self->pbdev, &hsv);

    // Save default settings
    pb_color_map_save_default(&self->color_map);
    self->chroma_weight = 50;

    return MP_OBJ_FROM_PTR(self);
}

// pybricks._common.ColorSensor.hsv
STATIC mp_obj_t pupdevices_ColorSensor_hsv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    // Create color object
    pb_type_Color_obj_t *color = pb_type_Color_new_empty();

    // Get either reflected or ambient HSV
    if (mp_obj_is_true(surface_in)) {
        pupdevices_ColorSensor__get_hsv_reflected(self->pbdev, &color->hsv);
    } else {
        pupdevices_ColorSensor__get_hsv_ambient(self->pbdev, &color->hsv);
    }

    // Return color
    return MP_OBJ_FROM_PTR(color);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorSensor_hsv_obj, 1, pupdevices_ColorSensor_hsv);

// pybricks._common.ColorSensor.color
STATIC mp_obj_t pupdevices_ColorSensor_color(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    // Get either reflected or ambient HSV
    pbio_color_hsv_t hsv;
    if (mp_obj_is_true(surface_in)) {
        pupdevices_ColorSensor__get_hsv_reflected(self->pbdev, &hsv);
    } else {
        pupdevices_ColorSensor__get_hsv_ambient(self->pbdev, &hsv);
    }

    // Get and return discretized color
    return pb_color_map_get_color(&self->color_map, &hsv, self->chroma_weight);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorSensor_color_obj, 1, pupdevices_ColorSensor_color);

// pybricks.pupdevices.ColorSensor.reflection
STATIC mp_obj_t pupdevices_ColorSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Get reflection from average RGB reflection, which ranges from 0 to 3*1024
    int32_t data[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_SENSOR__RGB_I, data);

    // Return value as reflection
    return mp_obj_new_int((data[0] + data[1] + data[2]) * 100 / 3072);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorSensor_reflection_obj, pupdevices_ColorSensor_reflection);

// pybricks.pupdevices.ColorSensor.ambient
STATIC mp_obj_t pupdevices_ColorSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Get ambient from "V" in SHSV, which ranges from 0 to 10000
    int32_t data[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_SENSOR__SHSV, data);

    // Return scaled to 100.
    return pb_obj_new_fraction(data[2], 100);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorSensor_ambient_obj, pupdevices_ColorSensor_ambient);

// dir(pybricks.pupdevices.ColorSensor)
STATIC const mp_rom_map_elem_t pupdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&pupdevices_ColorSensor_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&pupdevices_ColorSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_ColorSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&pupdevices_ColorSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_detectable_colors),   MP_ROM_PTR(&pb_ColorSensor_detectable_colors_obj)                    },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorSensor_locals_dict, pupdevices_ColorSensor_locals_dict_table);

STATIC const pb_attr_dict_entry_t pupdevices_ColorSensor_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_lights, pupdevices_ColorSensor_obj_t, lights),
};

// type(pybricks.pupdevices.ColorSensor)
const pb_obj_with_attr_type_t pb_type_pupdevices_ColorSensor = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = MP_QSTR_ColorSensor,
        .make_new = pupdevices_ColorSensor_make_new,
        .attr = pb_attribute_handler,
        .locals_dict = (mp_obj_dict_t *)&pupdevices_ColorSensor_locals_dict,
    },
    .attr_dict = pupdevices_ColorSensor_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(pupdevices_ColorSensor_attr_dict),
};

#endif // PYBRICKS_PY_PUPDEVICES
