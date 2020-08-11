// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include <pbio/light.h>
#include <pbio/color.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

// pybricks._common.ColorLight class object
typedef struct _common_ColorLight_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} common_ColorLight_obj_t;

// pybricks._common.ColorLight.on
STATIC mp_obj_t common_ColorLight_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse arguments
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_ColorLight_obj_t, self,
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_INT(brightness, 100));

    if (color == mp_const_none) {
        color = pb_const_color_black;
    }

    pbio_color_t color_id = pb_type_enum_get_value(color, &pb_enum_type_Color);

    mp_int_t bright = pb_obj_get_pct(brightness);

    // TODO: Brightness control is not yet implemented
    if (bright != 100) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    if (!self->pbdev) {
        // No external device, so assume command is for the internal light
        pb_assert(pbio_light_on(PBIO_PORT_SELF, color_id));
    } else {
        pb_device_color_light_on(self->pbdev, color_id);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_ColorLight_on_obj, 1, common_ColorLight_on);

// pybricks._common.ColorLight.off
STATIC mp_obj_t common_ColorLight_off(mp_obj_t self_in) {
    common_ColorLight_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Turn the light off, using the command specific to the device.
    if (!self->pbdev) {
        // No external device, so assume command is for the internal light
        pb_assert(pbio_light_off(PBIO_PORT_SELF));
    } else {
        pb_device_color_light_on(self->pbdev, PBIO_COLOR_NONE);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_ColorLight_off_obj, common_ColorLight_off);

// dir(pybricks.builtins.ColorLight)
STATIC const mp_rom_map_elem_t common_ColorLight_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&common_ColorLight_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&common_ColorLight_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(common_ColorLight_locals_dict, common_ColorLight_locals_dict_table);

// type(pybricks.builtins.ColorLight)
STATIC const mp_obj_type_t pb_type_ColorLight = {
    { &mp_type_type },
    .name = MP_QSTR_ColorLight,
    .locals_dict = (mp_obj_dict_t *)&common_ColorLight_locals_dict,
};

// pybricks._common.ColorLight.__init__
mp_obj_t common_ColorLight_obj_make_new(pb_device_t *pbdev) {
    // Create new light instance
    common_ColorLight_obj_t *light = m_new_obj(common_ColorLight_obj_t);
    // Set type and iodev
    light->base.type = &pb_type_ColorLight;
    light->pbdev = pbdev;
    return light;
}

#endif // PYBRICKS_PY_COMMON
