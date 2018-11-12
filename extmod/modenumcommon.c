#include <pbio/light.h>

#include "py/obj.h"

#include "pbobj.h"

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
