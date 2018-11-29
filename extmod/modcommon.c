#include <pbio/light.h>
#include <pbio/button.h>

#include "py/obj.h"

#include "pbobj.h"
#include "pberror.h"

/* Color enum */

STATIC const mp_rom_map_elem_t pb_Color_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_black),   MP_ROM_INT(PBIO_LIGHT_COLOR_BLACK)  },
    { MP_ROM_QSTR(MP_QSTR_purple),  MP_ROM_INT(PBIO_LIGHT_COLOR_PURPLE) },
    { MP_ROM_QSTR(MP_QSTR_blue),    MP_ROM_INT(PBIO_LIGHT_COLOR_BLUE)   },
    { MP_ROM_QSTR(MP_QSTR_green),   MP_ROM_INT(PBIO_LIGHT_COLOR_GREEN)  },
    { MP_ROM_QSTR(MP_QSTR_yellow),  MP_ROM_INT(PBIO_LIGHT_COLOR_YELLOW) },
    { MP_ROM_QSTR(MP_QSTR_orange),  MP_ROM_INT(PBIO_LIGHT_COLOR_ORANGE) },
    { MP_ROM_QSTR(MP_QSTR_red),     MP_ROM_INT(PBIO_LIGHT_COLOR_RED)    },
    { MP_ROM_QSTR(MP_QSTR_white),   MP_ROM_INT(PBIO_LIGHT_COLOR_WHITE)  },
    { MP_ROM_QSTR(MP_QSTR_brown),   MP_ROM_INT(PBIO_LIGHT_COLOR_BROWN)  },
};
PB_DEFINE_CONST_ENUM(pb_Color_enum, pb_Color_enum_table);

/* Generic button enum */

// TODO: Discuss using 10 unique numbers at PBIO instead of 8 with overlap

STATIC const mp_rom_map_elem_t pb_Button_enum_table[] = {
#if defined(PYBRICKS_BRICK_EV3)
    { MP_ROM_QSTR(MP_QSTR_up),          MP_ROM_INT(PBIO_BUTTON_UP)      },
    { MP_ROM_QSTR(MP_QSTR_down),        MP_ROM_INT(PBIO_BUTTON_DOWN)    },
    { MP_ROM_QSTR(MP_QSTR_left),        MP_ROM_INT(PBIO_BUTTON_LEFT)    },
    { MP_ROM_QSTR(MP_QSTR_right),       MP_ROM_INT(PBIO_BUTTON_RIGHT)   },
    { MP_ROM_QSTR(MP_QSTR_center),      MP_ROM_INT(PBIO_BUTTON_CENTER)  },
    { MP_ROM_QSTR(MP_QSTR_back),        MP_ROM_INT(PBIO_BUTTON_STOP)    },
    { MP_ROM_QSTR(MP_QSTR_left_up),     MP_ROM_INT(PBIO_BUTTON_UP)      },
    { MP_ROM_QSTR(MP_QSTR_left_down),   MP_ROM_INT(PBIO_BUTTON_DOWN)    },
    { MP_ROM_QSTR(MP_QSTR_right_up),    MP_ROM_INT(PBIO_BUTTON_UP2)     },
    { MP_ROM_QSTR(MP_QSTR_right_down),  MP_ROM_INT(PBIO_BUTTON_DOWN2)   },
    { MP_ROM_QSTR(MP_QSTR_beacon),      MP_ROM_INT(PBIO_BUTTON_UP)      },
#endif //PYBRICKS_BRICK_EV3
};
PB_DEFINE_CONST_ENUM(pb_Button_enum, pb_Button_enum_table);


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
