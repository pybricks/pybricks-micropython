// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/light.h>
#include <pbio/button.h>

#include "py/obj.h"

#include "pberror.h"

/* User status light functions */


STATIC mp_obj_t colorlight_color(mp_obj_t color) {

    pbio_light_color_t color_id = MP_OBJ_IS_TYPE(color, &mp_type_NoneType) ?
        PBIO_LIGHT_COLOR_NONE:
        mp_obj_get_int(color);

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
MP_DEFINE_CONST_FUN_OBJ_1(colorlight_color_obj, colorlight_color);


STATIC mp_obj_t colorlight_off() {

    pb_assert(pbio_light_off(PBIO_PORT_SELF));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(colorlight_off_obj, colorlight_off);


STATIC mp_obj_t colorlight_brightness(size_t n_args, const mp_obj_t *args) {

    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(colorlight_brightness_obj, 3, 3, colorlight_brightness);



STATIC const mp_rom_map_elem_t colorlight_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_colorlight)       },
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&colorlight_color_obj)     },
    { MP_ROM_QSTR(MP_QSTR_off),         MP_ROM_PTR(&colorlight_off_obj)       },
    { MP_ROM_QSTR(MP_QSTR_brightness),  MP_ROM_PTR(&colorlight_brightness_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_module_colorlight_globals, colorlight_globals_table);

const mp_obj_module_t pb_module_colorlight = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_colorlight_globals,
};


