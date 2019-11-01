// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/light.h>
#include <pbio/button.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "modlight.h"

#include "modparameters.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// Deprecated, dropped once lights are controlled through ColorLight instance
STATIC mp_obj_t colorlight_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_INT(brightness, 100)
    );

    if (color == mp_const_none) {
        color = MP_OBJ_FROM_PTR(&pb_const_black);
    }

    pbio_light_color_t color_id = enum_get_value_maybe(color, &pb_enum_type_Color);

    mp_int_t bright = pb_obj_get_int(brightness);
    bright = bright < 0 ? 0 : bright > 100 ? 100: bright;

    if (bright != 100) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    if (color_id < PBIO_LIGHT_COLOR_NONE || color_id > PBIO_LIGHT_COLOR_PURPLE) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    if (color_id == PBIO_LIGHT_COLOR_NONE || color_id == PBIO_LIGHT_COLOR_BLACK) {
        pb_assert(pbio_light_off(PBIO_PORT_SELF));
    }
    else {
        pb_assert(pbio_light_on(PBIO_PORT_SELF, color_id));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(colorlight_on_obj, 0, colorlight_on);

// Deprecated, dropped once lights are controlled through ColorLight instance
STATIC mp_obj_t colorlight_off() {

    pb_assert(pbio_light_off(PBIO_PORT_SELF));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(colorlight_off_obj, colorlight_off);

// Deprecated, dropped once lights are controlled through ColorLight instance
STATIC const mp_rom_map_elem_t colorlight_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_colorlight)       },
    { MP_ROM_QSTR(MP_QSTR_on),       MP_ROM_PTR(&colorlight_on_obj)     },
    { MP_ROM_QSTR(MP_QSTR_off),         MP_ROM_PTR(&colorlight_off_obj)       },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_colorlight_globals, colorlight_globals_table);

// Deprecated, dropped once lights are controlled through ColorLight instance
const mp_obj_module_t pb_module_colorlight = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_colorlight_globals,
};

// pybricks.builtins.Light class object
typedef struct _light_Light_obj_t {
    mp_obj_base_t base;
    pbio_lightdev_t dev;
} light_Light_obj_t;

// pybricks.builtins.Light.on
STATIC mp_obj_t light_Light_on(mp_obj_t self_in) {
    light_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->dev.id == PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR) {
        int16_t unused;
        pb_assert(ev3device_get_values_at_mode(self->dev.ev3iodev,
                                               PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM,
                                               &unused));
    }
    else {
        pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(light_Light_on_obj, light_Light_on);

// pybricks.builtins.ColorLight.on
STATIC mp_obj_t light_ColorLight_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_INT(brightness, 100)
    );
    light_Light_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (color == mp_const_none) {
        color = MP_OBJ_FROM_PTR(&pb_const_black);
    }

    pbio_light_color_t color_id = enum_get_value_maybe(color, &pb_enum_type_Color);

    mp_int_t bright = pb_obj_get_int(brightness);
    bright = bright < 0 ? 0 : bright > 100 ? 100: bright;

    // TODO: Brightness control is not yet implemented
    if (bright != 100) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    if (self->dev.id == PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR) {
        // TODO
    }
    else {
        // No external device, so assume command is for the internal light
        pb_assert(pbio_light_on(PBIO_PORT_SELF, color_id));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(light_ColorLight_on_obj, 0, light_ColorLight_on);

// pybricks.builtins.Light.off
// pybricks.builtins.LightArray.off
// pybricks.builtins.ColorLight.off
STATIC mp_obj_t light_Light_off(mp_obj_t self_in) {
    light_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->dev.id == PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR) {
        int16_t unused;
        pb_assert(ev3device_get_values_at_mode(self->dev.ev3iodev,
                                               PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM,
                                               &unused));
    }
    else {
        // No external device, so assume command is for the internal light
        pb_assert(pbio_light_off(PBIO_PORT_SELF));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(light_Light_off_obj, light_Light_off);

// dir(pybricks.builtins.Light)
STATIC const mp_rom_map_elem_t light_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&light_Light_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_on2  ), MP_ROM_PTR(&light_ColorLight_on_obj) }, // Fixme: move to ColorLight
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&light_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(light_Light_locals_dict, light_Light_locals_dict_table);

// type(pybricks.builtins.Light)
const mp_obj_type_t light_Light_type = {
    { &mp_type_type },
    .name = MP_QSTR_Light,
    .locals_dict = (mp_obj_dict_t*)&light_Light_locals_dict,
};

mp_obj_t light_Light_obj_make_new(pbio_lightdev_t dev, const mp_obj_type_t *type) {
    // Create new light instance
    light_Light_obj_t *light = m_new_obj(light_Light_obj_t);
    // Set type and iodev
    light->base.type = type;
    light->dev = dev;
    return light;
}
