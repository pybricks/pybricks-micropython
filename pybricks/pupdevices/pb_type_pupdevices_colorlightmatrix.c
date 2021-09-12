// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_conversions.h>
#include <pybricks/util_pb/pb_device.h>

// Class structure for ColorLightMatrix
typedef struct _pupdevices_ColorLightMatrix_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} pupdevices_ColorLightMatrix_obj_t;

// pybricks.pupdevices.ColorLightMatrix.__init__
STATIC mp_obj_t pupdevices_ColorLightMatrix_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorLightMatrix_obj_t *self = m_new_obj(pupdevices_ColorLightMatrix_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevice
    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ColorLightMatrix._get_color_id
STATIC uint8_t get_color_id(mp_obj_t color_in) {

    // Assert type and get hsv.
    const pbio_color_hsv_t *hsv = pb_type_Color_get_hsv(color_in);

    // Brightness is defined in 10 increments.
    uint8_t brightness = hsv->v / 10;

    // For low saturation, assume grayscale.
    if (hsv->s < 30) {
        // Brightness 1 is broken (shows faint red), so assume 0.
        if (brightness == 1) {
            brightness = 0;
        }
        // Base id for white is 10, plus 16 for each brightness step.
        return 10 + (brightness << 4);
    }

    // Everything else is rounded to nearest available hue.
    return pb_powered_up_color_id_from_hue(hsv->h) + (brightness << 4);
}

// pybricks.pupdevices.ColorLightMatrix.on
STATIC mp_obj_t pupdevices_ColorLightMatrix_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorLightMatrix_obj_t, self,
        PB_ARG_REQUIRED(colors));

    int32_t color_ids[9];

    // One color object, apply to all.
    if (mp_obj_is_type(colors_in, &pb_type_Color)) {
        color_ids[0] = get_color_id(colors_in);
        for (int i = 1; i < 9; i++) {
            color_ids[i] = color_ids[0];
        }
    }
    // Sequence of 9 colors.
    else {
        mp_obj_iter_buf_t iter_buf;
        mp_obj_t colors_iter = mp_getiter(colors_in, &iter_buf);
        for (int i = 0; i < 9; i++) {
            color_ids[i] = get_color_id(mp_iternext(colors_iter));
        }
    }

    // Activate all colors.
    pb_device_set_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O, color_ids, MP_ARRAY_SIZE(color_ids));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorLightMatrix_on_obj, 1, pupdevices_ColorLightMatrix_on);

// pybricks.pupdevices.ColorLightMatrix.off
STATIC mp_obj_t pupdevices_ColorLightMatrix_off(mp_obj_t self_in) {
    pupdevices_ColorLightMatrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Turn off all pixels.
    int32_t color_ids[9] = {0};
    pb_device_set_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O, color_ids, MP_ARRAY_SIZE(color_ids));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorLightMatrix_off_obj, pupdevices_ColorLightMatrix_off);

// dir(pybricks.pupdevices.ColorLightMatrix)
STATIC const mp_rom_map_elem_t pupdevices_ColorLightMatrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on),       MP_ROM_PTR(&pupdevices_ColorLightMatrix_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off),      MP_ROM_PTR(&pupdevices_ColorLightMatrix_off_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorLightMatrix_locals_dict, pupdevices_ColorLightMatrix_locals_dict_table);

// type(pybricks.pupdevices.ColorLightMatrix)
const mp_obj_type_t pb_type_pupdevices_ColorLightMatrix = {
    { &mp_type_type },
    .name = MP_QSTR_ColorLightMatrix,
    .make_new = pupdevices_ColorLightMatrix_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_ColorLightMatrix_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
