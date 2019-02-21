// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbio/light.h>
#include <pbio/button.h>

#include "py/obj.h"

#include "pberror.h"

/* User status light functions */

STATIC mp_obj_t hub_set_light(mp_obj_t color) {

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
MP_DEFINE_CONST_FUN_OBJ_1(hub_set_light_obj, hub_set_light);
