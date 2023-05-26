// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_conversions.h>

// Class structure for ColorLightMatrix
typedef struct _pupdevices_ColorLightMatrix_obj_t {
    pb_pupdevices_obj_base_t pup_base;
} pupdevices_ColorLightMatrix_obj_t;

// pybricks.pupdevices.ColorLightMatrix.__init__
STATIC mp_obj_t pupdevices_ColorLightMatrix_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorLightMatrix_obj_t *self = mp_obj_malloc(pupdevices_ColorLightMatrix_obj_t, type);
    pb_pupdevices_init_class(&self->pup_base, port_in, PBIO_IODEV_TYPE_ID_TECHNIC_COLOR_LIGHT_MATRIX);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ColorLightMatrix._get_color_id
STATIC uint8_t get_color_id(mp_obj_t color_in) {

    // Assert type and get hsv.
    const pbio_color_hsv_t *hsv = pb_type_Color_get_hsv(color_in);

    // Brightness is defined in 10 increments.
    uint8_t brightness = hsv->v / 10;
    pb_powered_up_color_id_t color;

    // For low saturation, assume grayscale.
    if (hsv->s < 30) {
        // Brightness 1 is broken (shows faint red), so assume 0.
        if (brightness == 1) {
            brightness = 0;
        }
        color = PB_PUP_COLOR_ID_WHITE;
    } else {
        // Everything else is rounded to nearest available hue.
        color = pb_powered_up_color_id_from_hue(hsv->h);
    }

    // The light matrix data format is a 4-bit brightness in the MSBs and a
    // 4-bit color ID in the LSBs.
    return (brightness << 4) | color;
}

// pybricks.pupdevices.ColorLightMatrix.on
STATIC mp_obj_t pupdevices_ColorLightMatrix_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorLightMatrix_obj_t, self,
        PB_ARG_REQUIRED(colors));

    int8_t color_ids[9];

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
    return pb_pupdevices_set_data(&self->pup_base, PBIO_IODEV_MODE_PUP_COLOR_LIGHT_MATRIX__PIX_O, color_ids);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorLightMatrix_on_obj, 1, pupdevices_ColorLightMatrix_on);

// pybricks.pupdevices.ColorLightMatrix.off
STATIC mp_obj_t pupdevices_ColorLightMatrix_off(mp_obj_t self_in) {
    const mp_obj_t pos_args[] = { self_in, MP_OBJ_FROM_PTR(&pb_Color_NONE_obj) };
    return pupdevices_ColorLightMatrix_on(MP_ARRAY_SIZE(pos_args), pos_args, NULL);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorLightMatrix_off_obj, pupdevices_ColorLightMatrix_off);

// dir(pybricks.pupdevices.ColorLightMatrix)
STATIC const mp_rom_map_elem_t pupdevices_ColorLightMatrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on),       MP_ROM_PTR(&pupdevices_ColorLightMatrix_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off),      MP_ROM_PTR(&pupdevices_ColorLightMatrix_off_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorLightMatrix_locals_dict, pupdevices_ColorLightMatrix_locals_dict_table);

// type(pybricks.pupdevices.ColorLightMatrix)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_ColorLightMatrix,
    MP_QSTR_ColorLightMatrix,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_ColorLightMatrix_make_new,
    locals_dict, &pupdevices_ColorLightMatrix_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
