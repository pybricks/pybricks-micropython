// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_device.h>

// pybricks.nxtdevices.ColorSensor class object. Note: first two members must match pb_ColorSensor_obj_t
typedef struct _nxtdevices_ColorSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t color_map;
    mp_obj_t light;
    pb_device_t *pbdev;
} nxtdevices_ColorSensor_obj_t;

STATIC mp_obj_t nxtdevices_ColorSensor_light_on(void *context, const pbio_color_hsv_t *hsv) {
    pb_device_t *pbdev = context;
    uint8_t mode;

    if (hsv->h == PBIO_COLOR_HUE_RED) {
        mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_R;
    } else if (hsv->h == PBIO_COLOR_HUE_GREEN) {
        mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_G;
    } else if (hsv->h == PBIO_COLOR_HUE_BLUE) {
        mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_B;
    } else {
        mode = PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP_OFF;
    }
    int32_t unused;
    pb_device_get_values(pbdev, mode, &unused);
    return mp_const_none;
}

// pybricks.nxtdevices.ColorSensor.__init__
STATIC mp_obj_t nxtdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_ColorSensor_obj_t *self = mp_obj_malloc(nxtdevices_ColorSensor_obj_t, type);

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR);

    // Create an instance of the Light class
    self->light = pb_type_ColorLight_external_obj_new(self->pbdev, nxtdevices_ColorSensor_light_on);

    // Set the light color to red
    nxtdevices_ColorSensor_light_on(self->pbdev, &pb_Color_RED_obj.hsv);

    // Save default color settings
    pb_color_map_save_default(&self->color_map);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.ColorSensor.rgb
STATIC mp_obj_t nxtdevices_ColorSensor_rgb(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    mp_obj_t ret[3];
    for (uint8_t i = 0; i < 3; i++) {
        // scale rgb bytes to percentage
        ret[i] = mp_obj_new_int(all[i] * 101 / 256);
    }
    return mp_obj_new_tuple(3, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_rgb_obj, nxtdevices_ColorSensor_rgb);

// pybricks.nxtdevices.ColorSensor.reflection
STATIC mp_obj_t nxtdevices_ColorSensor_reflection(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the average of red, green, and blue reflection
    return mp_obj_new_int((all[0] + all[1] + all[2]) * 101 / (3 * 256));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_reflection_obj, nxtdevices_ColorSensor_reflection);

// pybricks.nxtdevices.ColorSensor.ambient
STATIC mp_obj_t nxtdevices_ColorSensor_ambient(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the ambient light
    return mp_obj_new_int(all[3]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_ambient_obj, nxtdevices_ColorSensor_ambient);

// pybricks.nxtdevices.ColorSensor.hsv
STATIC mp_obj_t nxtdevices_ColorSensor_hsv(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Read sensor data
    int32_t all[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    pbio_color_rgb_t rgb = {
        .r = all[0],
        .g = all[1],
        .b = all[2],
    };

    // Create color object
    pb_type_Color_obj_t *color = pb_type_Color_new_empty();

    // Convert and store RGB as HSV
    pb_color_map_rgb_to_hsv(&rgb, &color->hsv);

    // Return color
    return MP_OBJ_FROM_PTR(color);
}
MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_hsv_obj, nxtdevices_ColorSensor_hsv);

// pybricks.nxtdevices.ColorSensor.color
STATIC mp_obj_t nxtdevices_ColorSensor_color(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t all[4];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);

    pbio_color_hsv_t hsv;
    pbio_color_rgb_t rgb = {
        .r = all[0],
        .g = all[1],
        .b = all[2],
    };
    pb_color_map_rgb_to_hsv(&rgb, &hsv);

    // Get and return discretized color based on HSV
    return pb_color_map_get_color(&self->color_map, &hsv);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_color_obj, nxtdevices_ColorSensor_color);

STATIC const pb_attr_dict_entry_t nxtdevices_ColorSensor_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, nxtdevices_ColorSensor_obj_t, light),
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.nxtdevices.ColorSensor)
STATIC const mp_rom_map_elem_t nxtdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_hsv),        MP_ROM_PTR(&nxtdevices_ColorSensor_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_rgb),        MP_ROM_PTR(&nxtdevices_ColorSensor_rgb_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_ambient),    MP_ROM_PTR(&nxtdevices_ColorSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_ColorSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_color),      MP_ROM_PTR(&nxtdevices_ColorSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_detectable_colors),  MP_ROM_PTR(&pb_ColorSensor_detectable_colors_obj)                    },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_ColorSensor_locals_dict, nxtdevices_ColorSensor_locals_dict_table);

// type(pybricks.nxtdevices.ColorSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_ColorSensor,
    MP_QSTR_ColorSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_ColorSensor_make_new,
    attr, pb_attribute_handler,
    protocol, nxtdevices_ColorSensor_attr_dict,
    locals_dict, &nxtdevices_ColorSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
