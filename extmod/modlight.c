// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/light.h>
#include <pbio/button.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "modlight.h"
#include "pbdevice.h"

#include "modlight.h"
#include "modparameters.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.builtins.Light class object
typedef struct _light_Light_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} light_Light_obj_t;

// pybricks.builtins.Light.on
STATIC mp_obj_t light_Light_on(mp_obj_t self_in) {
    pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(light_Light_on_obj, light_Light_on);

// pybricks.builtins.ColorLight.on
STATIC mp_obj_t light_ColorLight_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse arguments
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        light_Light_obj_t, self,
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_INT(brightness, 100)
    );

    if (color == mp_const_none) {
        color = MP_OBJ_FROM_PTR(&pb_const_black);
    }

    pbio_light_color_t color_id = pb_type_enum_get_value(color, &pb_enum_type_Color);

    mp_int_t bright = pb_obj_get_int(brightness);
    bright = bright < 0 ? 0 : bright > 100 ? 100: bright;

    // TODO: Brightness control is not yet implemented
    if (bright != 100) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    if (!self->pbdev) {
        // No external device, so assume command is for the internal light
        pb_assert(pbio_light_on(PBIO_PORT_SELF, color_id));
    }
    else {
        pbdevice_color_light_on(self->pbdev, color_id);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(light_ColorLight_on_obj, 0, light_ColorLight_on);

// pybricks.builtins.Light.off
// pybricks.builtins.LightArray.off
// pybricks.builtins.ColorLight.off
STATIC mp_obj_t light_Light_off(mp_obj_t self_in) {
    light_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Turn the light off, using the command specific to the device. 
    if (!self->pbdev) {
        // No external device, so assume command is for the internal light
        pb_assert(pbio_light_off(PBIO_PORT_SELF));
    }
    else {
        pbdevice_color_light_on(self->pbdev, PBIO_LIGHT_COLOR_NONE);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(light_Light_off_obj, light_Light_off);

// dir(pybricks.builtins.Light)
STATIC const mp_rom_map_elem_t light_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&light_Light_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&light_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(light_Light_locals_dict, light_Light_locals_dict_table);

// type(pybricks.builtins.Light)
const mp_obj_type_t light_Light_type = {
    { &mp_type_type },
    .name = MP_QSTR_Light,
    .locals_dict = (mp_obj_dict_t*)&light_Light_locals_dict,
};

// dir(pybricks.builtins.ColorLight)
STATIC const mp_rom_map_elem_t light_ColorLight_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&light_ColorLight_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&light_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(light_ColorLight_locals_dict, light_ColorLight_locals_dict_table);

// type(pybricks.builtins.ColorLight)
const mp_obj_type_t light_ColorLight_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorLight,
    .locals_dict = (mp_obj_dict_t*)&light_ColorLight_locals_dict,
};

mp_obj_t light_Light_obj_make_new(pbdevice_t *pbdev, const mp_obj_type_t *type) {
    // Create new light instance
    light_Light_obj_t *light = m_new_obj(light_Light_obj_t);
    // Set type and iodev
    light->base.type = type;
    light->pbdev = pbdev;
    return light;
}
