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

#include "modparameters.h"
#include "pb_type_enum.h"
#include "pberror.h"
#include "pbobj.h"

#if PYBRICKS_PY_PARAMETERS

/* Brick/Hub Port enum */

/* Port enum */

STATIC const mp_rom_map_elem_t pb_enum_Port_table[] = {
#if PBDRV_CONFIG_HAS_PORT_A
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_A, PBIO_PORT_A),
#endif
#if PBDRV_CONFIG_HAS_PORT_B
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_B, PBIO_PORT_B),
#endif
#if PBDRV_CONFIG_HAS_PORT_C
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_C, PBIO_PORT_C),
#endif
#if PBDRV_CONFIG_HAS_PORT_D
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_D, PBIO_PORT_D),
#endif
#if PBDRV_CONFIG_HAS_PORT_E
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_E, PBIO_PORT_E),
#endif
#if PBDRV_CONFIG_HAS_PORT_F
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_F, PBIO_PORT_F),
#endif
#if PBDRV_CONFIG_HAS_PORT_1
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S1, PBIO_PORT_1),
#endif
#if PBDRV_CONFIG_HAS_PORT_2
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S2, PBIO_PORT_2),
#endif
#if PBDRV_CONFIG_HAS_PORT_3
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S3, PBIO_PORT_3),
#endif
#if PBDRV_CONFIG_HAS_PORT_4
    PB_ROM_ENUM_MEMBER(pb_enum_type_Port, MP_QSTR_S4, PBIO_PORT_4),
#endif
};
PB_DEFINE_ENUM(pb_enum_type_Port, MP_QSTR_Port, pb_enum_Port_table);

/* Stop enum */

const pb_obj_enum_member_t pb_Stop_COAST_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_COAST,
    .value = PBIO_ACTUATION_COAST
};

const pb_obj_enum_member_t pb_Stop_BRAKE_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_BRAKE,
    .value = PBIO_ACTUATION_BRAKE
};

const pb_obj_enum_member_t pb_Stop_HOLD_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_HOLD,
    .value = PBIO_ACTUATION_HOLD
};

STATIC const mp_rom_map_elem_t pb_enum_Stop_table[] = {
    { MP_ROM_QSTR(MP_QSTR_COAST),   MP_ROM_PTR(&pb_Stop_COAST_obj) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE),   MP_ROM_PTR(&pb_Stop_BRAKE_obj) },
    { MP_ROM_QSTR(MP_QSTR_HOLD),    MP_ROM_PTR(&pb_Stop_HOLD_obj)  },
};
PB_DEFINE_ENUM(pb_enum_type_Stop, MP_QSTR_Stop, pb_enum_Stop_table);

/* Direction enum */

const pb_obj_enum_member_t pb_Direction_CLOCKWISE_obj = {
    {&pb_enum_type_Direction},
    .name = MP_QSTR_CLOCKWISE,
    .value = PBIO_DIRECTION_CLOCKWISE
};

const pb_obj_enum_member_t pb_Direction_COUNTERCLOCKWISE_obj = {
    {&pb_enum_type_Direction},
    .name = MP_QSTR_COUNTERCLOCKWISE,
    .value = PBIO_DIRECTION_COUNTERCLOCKWISE
};

STATIC const mp_rom_map_elem_t pb_enum_Direction_table[] = {
    { MP_ROM_QSTR(MP_QSTR_CLOCKWISE),         MP_ROM_PTR(&pb_Direction_CLOCKWISE_obj)        },
    { MP_ROM_QSTR(MP_QSTR_COUNTERCLOCKWISE),  MP_ROM_PTR(&pb_Direction_COUNTERCLOCKWISE_obj) },
};
PB_DEFINE_ENUM(pb_enum_type_Direction, MP_QSTR_Direction, pb_enum_Direction_table);

/* Color enum */

const pb_obj_enum_member_t pb_Color_BLACK_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BLACK,
    .value = PBIO_LIGHT_COLOR_BLACK
};

const pb_obj_enum_member_t pb_Color_PURPLE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_PURPLE,
    .value = PBIO_LIGHT_COLOR_PURPLE
};

const pb_obj_enum_member_t pb_Color_BLUE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BLUE,
    .value = PBIO_LIGHT_COLOR_BLUE
};

const pb_obj_enum_member_t pb_Color_GREEN_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_GREEN,
    .value = PBIO_LIGHT_COLOR_GREEN
};

const pb_obj_enum_member_t pb_Color_YELLOW_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_YELLOW,
    .value = PBIO_LIGHT_COLOR_YELLOW
};

const pb_obj_enum_member_t pb_Color_ORANGE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_ORANGE,
    .value = PBIO_LIGHT_COLOR_ORANGE
};

const pb_obj_enum_member_t pb_Color_RED_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_RED,
    .value = PBIO_LIGHT_COLOR_RED
};

const pb_obj_enum_member_t pb_Color_WHITE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_WHITE,
    .value = PBIO_LIGHT_COLOR_WHITE
};

const pb_obj_enum_member_t pb_Color_BROWN_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BROWN,
    .value = PBIO_LIGHT_COLOR_BROWN
};

STATIC const mp_rom_map_elem_t pb_Color_enum_table[] = {
    { MP_ROM_QSTR(MP_QSTR_BLACK),   MP_ROM_PTR(&pb_Color_BLACK_obj)  },
    { MP_ROM_QSTR(MP_QSTR_PURPLE),  MP_ROM_PTR(&pb_Color_PURPLE_obj) },
    { MP_ROM_QSTR(MP_QSTR_BLUE),    MP_ROM_PTR(&pb_Color_BLUE_obj)   },
    { MP_ROM_QSTR(MP_QSTR_GREEN),   MP_ROM_PTR(&pb_Color_GREEN_obj)  },
    { MP_ROM_QSTR(MP_QSTR_YELLOW),  MP_ROM_PTR(&pb_Color_YELLOW_obj) },
    { MP_ROM_QSTR(MP_QSTR_ORANGE),  MP_ROM_PTR(&pb_Color_ORANGE_obj) },
    { MP_ROM_QSTR(MP_QSTR_RED),     MP_ROM_PTR(&pb_Color_RED_obj)    },
    { MP_ROM_QSTR(MP_QSTR_WHITE),   MP_ROM_PTR(&pb_Color_WHITE_obj)  },
    { MP_ROM_QSTR(MP_QSTR_BROWN),   MP_ROM_PTR(&pb_Color_BROWN_obj)  },
};
PB_DEFINE_ENUM(pb_enum_type_Color, MP_QSTR_Color, pb_Color_enum_table);

/* Button enum */

const pb_obj_enum_member_t pb_Button_UP_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_UP,
    .value = PBIO_BUTTON_UP
};

const pb_obj_enum_member_t pb_Button_DOWN_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_DOWN,
    .value = PBIO_BUTTON_DOWN
};

const pb_obj_enum_member_t pb_Button_LEFT_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_LEFT,
    .value = PBIO_BUTTON_LEFT
};

const pb_obj_enum_member_t pb_Button_RIGHT_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_RIGHT,
    .value = PBIO_BUTTON_RIGHT
};

const pb_obj_enum_member_t pb_Button_CENTER_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_CENTER,
    .value = PBIO_BUTTON_CENTER
};

const pb_obj_enum_member_t pb_Button_LEFT_UP_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_LEFT_UP,
    .value = PBIO_BUTTON_LEFT_UP
};

const pb_obj_enum_member_t pb_Button_LEFT_DOWN_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_LEFT_DOWN,
    .value = PBIO_BUTTON_LEFT_DOWN
};

const pb_obj_enum_member_t pb_Button_RIGHT_UP_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_RIGHT_UP,
    .value = PBIO_BUTTON_RIGHT_UP
};

const pb_obj_enum_member_t pb_Button_RIGHT_DOWN_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_RIGHT_DOWN,
    .value = PBIO_BUTTON_RIGHT_DOWN
};

const pb_obj_enum_member_t pb_Button_BEACON_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_BEACON,
    .value = PBIO_BUTTON_UP
};

STATIC const mp_rom_map_elem_t pb_enum_Button_table[] = {
    { MP_ROM_QSTR(MP_QSTR_UP),         MP_ROM_PTR(&pb_Button_UP_obj)        },
    { MP_ROM_QSTR(MP_QSTR_DOWN),       MP_ROM_PTR(&pb_Button_DOWN_obj)      },
    { MP_ROM_QSTR(MP_QSTR_LEFT),       MP_ROM_PTR(&pb_Button_LEFT_obj)      },
    { MP_ROM_QSTR(MP_QSTR_RIGHT),      MP_ROM_PTR(&pb_Button_RIGHT_obj)     },
    { MP_ROM_QSTR(MP_QSTR_CENTER),     MP_ROM_PTR(&pb_Button_CENTER_obj)    },
    { MP_ROM_QSTR(MP_QSTR_LEFT_UP),    MP_ROM_PTR(&pb_Button_LEFT_UP_obj)   },
    { MP_ROM_QSTR(MP_QSTR_LEFT_DOWN),  MP_ROM_PTR(&pb_Button_LEFT_DOWN_obj) },
    { MP_ROM_QSTR(MP_QSTR_RIGHT_UP),   MP_ROM_PTR(&pb_Button_RIGHT_UP_obj)  },
    { MP_ROM_QSTR(MP_QSTR_RIGHT_DOWN), MP_ROM_PTR(&pb_Button_RIGHT_DOWN_obj)},
    { MP_ROM_QSTR(MP_QSTR_BEACON),     MP_ROM_PTR(&pb_Button_BEACON_obj)    },
};
PB_DEFINE_ENUM(pb_enum_type_Button, MP_QSTR_Button, pb_enum_Button_table);

/*
parameters module tables
*/

STATIC const mp_rom_map_elem_t parameters_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_parameters)     },
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&pb_enum_type_Port)      },
    { MP_ROM_QSTR(MP_QSTR_Stop),        MP_ROM_PTR(&pb_enum_type_Stop)      },
    { MP_ROM_QSTR(MP_QSTR_Direction),   MP_ROM_PTR(&pb_enum_type_Direction) },
    { MP_ROM_QSTR(MP_QSTR_Color),       MP_ROM_PTR(&pb_enum_type_Color)     },
    { MP_ROM_QSTR(MP_QSTR_Button),      MP_ROM_PTR(&pb_enum_type_Button)    },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_parameters_globals, parameters_globals_table);

const mp_obj_module_t pb_module_parameters = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_parameters_globals,
};

#endif // PYBRICKS_PY_PARAMETERS
