// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES

#include <pbio/color.h>

#include <pbio/iodev.h>

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objtype.h"

#include <pybricks/util_pb/pb_device.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_pb/pb_error.h>

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>

// pybricks.nxtdevices.ColorSensor class object
typedef struct _nxtdevices_ColorSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    pb_device_t *pbdev;
} nxtdevices_ColorSensor_obj_t;

// pybricks.nxtdevices.ColorSensor.__init__
STATIC mp_obj_t nxtdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_ColorSensor_obj_t *self = m_new_obj(nxtdevices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR);

    // Create an instance of the Light class
    self->light = common_ColorLight_obj_make_new(self->pbdev);

    // Set the light color to red
    pb_device_color_light_on(self->pbdev, PBIO_COLOR_RED);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t color_obj(pbio_color_t color) {
    switch (color) {
        case PBIO_COLOR_RED:
            return pb_const_color_red;
        case PBIO_COLOR_GREEN:
            return pb_const_color_green;
        case PBIO_COLOR_BLUE:
            return pb_const_color_blue;
        case PBIO_COLOR_YELLOW:
            return pb_const_color_yellow;
        case PBIO_COLOR_BLACK:
            return pb_const_color_black;
        case PBIO_COLOR_WHITE:
            return pb_const_color_white;
        default:
            return mp_const_none;
    }
}

// pybricks.nxtdevices.ColorSensor.all
STATIC mp_obj_t nxtdevices_ColorSensor_all(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    mp_obj_t ret[5];
    for (uint8_t i = 0; i < 4; i++) {
        ret[i] = mp_obj_new_int(all[i]);
    }
    ret[4] = color_obj(all[4]);

    return mp_obj_new_tuple(5, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_all_obj, nxtdevices_ColorSensor_all);

// pybricks.nxtdevices.ColorSensor.rgb
STATIC mp_obj_t nxtdevices_ColorSensor_rgb(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    mp_obj_t ret[3];
    for (uint8_t i = 0; i < 3; i++) {
        ret[i] = mp_obj_new_int(all[i]);
    }
    return mp_obj_new_tuple(3, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_rgb_obj, nxtdevices_ColorSensor_rgb);

// pybricks.nxtdevices.ColorSensor.reflection
STATIC mp_obj_t nxtdevices_ColorSensor_reflection(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the average of red, green, and blue reflection
    return mp_obj_new_int((all[0] + all[1] + all[2]) / 3);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_reflection_obj, nxtdevices_ColorSensor_reflection);

// pybricks.nxtdevices.ColorSensor.ambient
STATIC mp_obj_t nxtdevices_ColorSensor_ambient(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the ambient light
    return mp_obj_new_int(all[3]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_ambient_obj, nxtdevices_ColorSensor_ambient);

// pybricks.nxtdevices.ColorSensor.color
STATIC mp_obj_t nxtdevices_ColorSensor_color(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the color ID
    return color_obj(all[4]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_color_obj, nxtdevices_ColorSensor_color);

// dir(pybricks.nxtdevices.ColorSensor)
STATIC const mp_rom_map_elem_t nxtdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_all),        MP_ROM_PTR(&nxtdevices_ColorSensor_all_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_rgb),        MP_ROM_PTR(&nxtdevices_ColorSensor_rgb_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_ambient),    MP_ROM_PTR(&nxtdevices_ColorSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_ColorSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_color),      MP_ROM_PTR(&nxtdevices_ColorSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_light),      MP_ROM_ATTRIBUTE_OFFSET(nxtdevices_ColorSensor_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_ColorSensor_locals_dict, nxtdevices_ColorSensor_locals_dict_table);

// type(pybricks.nxtdevices.ColorSensor)
const mp_obj_type_t pb_type_nxtdevices_ColorSensor = {
    { &mp_type_type },
    .name = MP_QSTR_ColorSensor,
    .make_new = nxtdevices_ColorSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_ColorSensor_locals_dict,
};

#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
