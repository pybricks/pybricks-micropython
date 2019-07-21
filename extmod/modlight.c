// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/light.h>
#include <pbio/button.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"


STATIC mp_obj_t colorlight_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_INT(brightness, 100)
    );

    pbio_light_color_t color_id = MP_OBJ_IS_TYPE(color, &mp_type_NoneType) ?
        PBIO_LIGHT_COLOR_NONE:
        mp_obj_get_int(color);


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

STATIC mp_obj_t colorlight_off() {

    pb_assert(pbio_light_off(PBIO_PORT_SELF));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(colorlight_off_obj, colorlight_off);


STATIC const mp_rom_map_elem_t colorlight_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_colorlight)       },
    { MP_ROM_QSTR(MP_QSTR_on),       MP_ROM_PTR(&colorlight_on_obj)     },
    { MP_ROM_QSTR(MP_QSTR_off),         MP_ROM_PTR(&colorlight_off_obj)       },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_colorlight_globals, colorlight_globals_table);

const mp_obj_module_t pb_module_colorlight = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_colorlight_globals,
};


