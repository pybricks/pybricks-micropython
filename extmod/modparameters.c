// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2018-2019 David Lechner
// Copyright (c) 2019 LEGO System A/S


#include <pbdrv/config.h>
#include <pbio/light.h>
#include <pbio/button.h>
#include <pbio/servo.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "modenum.h"
#include "pbobj.h"
#include "pberror.h"

/* Brick/Hub Port enum */

STATIC const mp_rom_map_elem_t pb_Port_enum_table[] = {
#if PBDRV_CONFIG_HAS_PORT_A
    { MP_ROM_QSTR(MP_QSTR_A),   MP_ROM_INT(PBIO_PORT_A) },
#endif
#if PBDRV_CONFIG_HAS_PORT_B
    { MP_ROM_QSTR(MP_QSTR_B),   MP_ROM_INT(PBIO_PORT_B) },
#endif
#if PBDRV_CONFIG_HAS_PORT_C
    { MP_ROM_QSTR(MP_QSTR_C),   MP_ROM_INT(PBIO_PORT_C) },
#endif
#if PBDRV_CONFIG_HAS_PORT_D
    { MP_ROM_QSTR(MP_QSTR_D),   MP_ROM_INT(PBIO_PORT_D) },
#endif
#if PBDRV_CONFIG_HAS_PORT_E
    { MP_ROM_QSTR(MP_QSTR_E),   MP_ROM_INT(PBIO_PORT_E) },
#endif
#if PBDRV_CONFIG_HAS_PORT_F
    { MP_ROM_QSTR(MP_QSTR_F),   MP_ROM_INT(PBIO_PORT_F) },
#endif
#if PBDRV_CONFIG_HAS_PORT_1
    { MP_ROM_QSTR(MP_QSTR_S1),  MP_ROM_INT(PBIO_PORT_1) },
#endif
#if PBDRV_CONFIG_HAS_PORT_2
    { MP_ROM_QSTR(MP_QSTR_S2),  MP_ROM_INT(PBIO_PORT_2) },
#endif
#if PBDRV_CONFIG_HAS_PORT_3
    { MP_ROM_QSTR(MP_QSTR_S3),  MP_ROM_INT(PBIO_PORT_3) },
#endif
#if PBDRV_CONFIG_HAS_PORT_4
    { MP_ROM_QSTR(MP_QSTR_S4),  MP_ROM_INT(PBIO_PORT_4) },
#endif
};
STATIC PB_DEFINE_CONST_ENUM(pb_Port_enum, pb_Port_enum_table);


/* Motor stop enum */

STATIC const mp_rom_map_elem_t motor_Stop_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_COAST),       MP_ROM_INT(PBIO_ACTUATION_COAST)   },
    { MP_ROM_QSTR(MP_QSTR_BRAKE),       MP_ROM_INT(PBIO_ACTUATION_BRAKE)   },
    { MP_ROM_QSTR(MP_QSTR_HOLD),        MP_ROM_INT(PBIO_ACTUATION_HOLD)    },
};
PB_DEFINE_CONST_ENUM(motor_Stop_enum, motor_Stop_enum_table);

/* Motor direction enum */

STATIC const mp_rom_map_elem_t motor_Direction_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_CLOCKWISE),        MP_ROM_INT(PBIO_DIRECTION_CLOCKWISE)        },
    { MP_ROM_QSTR(MP_QSTR_COUNTERCLOCKWISE), MP_ROM_INT(PBIO_DIRECTION_COUNTERCLOCKWISE) },
};
PB_DEFINE_CONST_ENUM(motor_Direction_enum, motor_Direction_enum_table);

/* Color enum */

const mp_obj_type_t pb_enum_type_Color;

// Color is just like the other enums, but we define entries manually
// instead of using the MP_ROM_ENUM_ELEM macro, so we can also return
// these constants from color sensors.
const pb_obj_enum_elem_t pb_const_black = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BLACK,
    .value = PBIO_LIGHT_COLOR_BLACK
};

const pb_obj_enum_elem_t pb_const_purple = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_PURPLE,
    .value = PBIO_LIGHT_COLOR_PURPLE
};

const pb_obj_enum_elem_t pb_const_blue = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BLUE,
    .value = PBIO_LIGHT_COLOR_BLUE
};

const pb_obj_enum_elem_t pb_const_green = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_GREEN,
    .value = PBIO_LIGHT_COLOR_GREEN
};

const pb_obj_enum_elem_t pb_const_yellow = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_YELLOW,
    .value = PBIO_LIGHT_COLOR_YELLOW
};

const pb_obj_enum_elem_t pb_const_orange = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_ORANGE,
    .value = PBIO_LIGHT_COLOR_ORANGE
};

const pb_obj_enum_elem_t pb_const_red = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_RED,
    .value = PBIO_LIGHT_COLOR_RED
};

const pb_obj_enum_elem_t pb_const_white = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_WHITE,
    .value = PBIO_LIGHT_COLOR_WHITE
};

const pb_obj_enum_elem_t pb_const_brown = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BROWN,
    .value = PBIO_LIGHT_COLOR_BROWN
};

STATIC const mp_rom_map_elem_t pb_Color_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_BLACK),   MP_ROM_PTR(&pb_const_black)  },
    { MP_ROM_QSTR(MP_QSTR_PURPLE),  MP_ROM_PTR(&pb_const_purple) },
    { MP_ROM_QSTR(MP_QSTR_BLUE),    MP_ROM_PTR(&pb_const_blue)   },
    { MP_ROM_QSTR(MP_QSTR_GREEN),   MP_ROM_PTR(&pb_const_green)  },
    { MP_ROM_QSTR(MP_QSTR_YELLOW),  MP_ROM_PTR(&pb_const_yellow) },
    { MP_ROM_QSTR(MP_QSTR_ORANGE),  MP_ROM_PTR(&pb_const_orange) },
    { MP_ROM_QSTR(MP_QSTR_RED),     MP_ROM_PTR(&pb_const_red)    },
    { MP_ROM_QSTR(MP_QSTR_WHITE),   MP_ROM_PTR(&pb_const_white)   },
    { MP_ROM_QSTR(MP_QSTR_BROWN),   MP_ROM_PTR(&pb_const_brown)   },
};
PB_DEFINE_ENUM(pb_enum_type_Color, MP_QSTR_Color, pb_Color_enum_table);

/* Generic button enum */

STATIC const mp_rom_map_elem_t pb_Button_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_UP),          MP_ROM_INT(PBIO_BUTTON_UP)         },
    { MP_ROM_QSTR(MP_QSTR_DOWN),        MP_ROM_INT(PBIO_BUTTON_DOWN)       },
    { MP_ROM_QSTR(MP_QSTR_LEFT),        MP_ROM_INT(PBIO_BUTTON_LEFT)       },
    { MP_ROM_QSTR(MP_QSTR_RIGHT),       MP_ROM_INT(PBIO_BUTTON_RIGHT)      },
    { MP_ROM_QSTR(MP_QSTR_CENTER),      MP_ROM_INT(PBIO_BUTTON_CENTER)     },
    { MP_ROM_QSTR(MP_QSTR_LEFT_UP),     MP_ROM_INT(PBIO_BUTTON_LEFT_UP)    },
    { MP_ROM_QSTR(MP_QSTR_LEFT_DOWN),   MP_ROM_INT(PBIO_BUTTON_LEFT_DOWN)  },
    { MP_ROM_QSTR(MP_QSTR_RIGHT_UP),    MP_ROM_INT(PBIO_BUTTON_RIGHT_UP)   },
    { MP_ROM_QSTR(MP_QSTR_RIGHT_DOWN),  MP_ROM_INT(PBIO_BUTTON_RIGHT_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_BEACON),      MP_ROM_INT(PBIO_BUTTON_UP)         },
};
PB_DEFINE_CONST_ENUM(pb_Button_enum, pb_Button_enum_table);

/*
parameters module tables
*/

STATIC const mp_rom_map_elem_t parameters_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_parameters)     },
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&pb_Port_enum)           },
    { MP_ROM_QSTR(MP_QSTR_Stop),        MP_ROM_PTR(&motor_Stop_enum)        },
    { MP_ROM_QSTR(MP_QSTR_Direction),   MP_ROM_PTR(&motor_Direction_enum)   },
    { MP_ROM_QSTR(MP_QSTR_Color),       MP_ROM_PTR(&pb_enum_type_Color)     },
    { MP_ROM_QSTR(MP_QSTR_Button),      MP_ROM_PTR(&pb_Button_enum)         },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_parameters_globals, parameters_globals_table);

const mp_obj_module_t pb_module_parameters = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_parameters_globals,
};
