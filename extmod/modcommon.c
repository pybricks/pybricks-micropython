#include <pbio/light.h>

#include "py/obj.h"

#include "pbobj.h"
#include "pberror.h"

/* Color enum */

STATIC const mp_rom_map_elem_t pb_Color_enum_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_black),     MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_BLACK)  },
    { MP_OBJ_NEW_QSTR(MP_QSTR_purple),    MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_PURPLE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_blue),      MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_BLUE)   },
    { MP_OBJ_NEW_QSTR(MP_QSTR_green),     MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_GREEN)  },
    { MP_OBJ_NEW_QSTR(MP_QSTR_yellow),    MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_YELLOW) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_orange),    MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_ORANGE) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_red),       MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_RED)    },
    { MP_OBJ_NEW_QSTR(MP_QSTR_white),     MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_WHITE)  },
    { MP_OBJ_NEW_QSTR(MP_QSTR_brown),     MP_OBJ_NEW_SMALL_INT(PBIO_LIGHT_COLOR_BROWN)  },
};
PB_DEFINE_CONST_ENUM(pb_Color_enum, pb_Color_enum_table);

/* User status light functions */

// TODO: left original hub_set_light as is for now, since this commit is just about adding low level light support for EV3
// However, in progress: TODO: allow None color, rgb tuple color, optional pattern enum arg, and use pb_assert

STATIC mp_obj_t hub_set_light(mp_obj_t color) {
    pbio_light_color_t color_id = mp_obj_get_int(color);
    if (color_id < PBIO_LIGHT_COLOR_NONE || color_id > PBIO_LIGHT_COLOR_PURPLE) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    if (color_id == PBIO_LIGHT_COLOR_NONE) {
        pbio_light_off(PBIO_PORT_SELF);
    }
    else {
        pbio_light_on(PBIO_PORT_SELF, color_id);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hub_set_light_obj, hub_set_light);
