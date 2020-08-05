// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/color.h>

#include <pybricks/parameters.h>
#include <pybricks/util_mp/pb_type_enum.h>

const pb_obj_enum_member_t pb_Color_BLACK_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BLACK,
    .value = PBIO_COLOR_BLACK
};

const pb_obj_enum_member_t pb_Color_PURPLE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_PURPLE,
    .value = PBIO_COLOR_PURPLE
};

const pb_obj_enum_member_t pb_Color_BLUE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BLUE,
    .value = PBIO_COLOR_BLUE
};

const pb_obj_enum_member_t pb_Color_GREEN_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_GREEN,
    .value = PBIO_COLOR_GREEN
};

const pb_obj_enum_member_t pb_Color_YELLOW_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_YELLOW,
    .value = PBIO_COLOR_YELLOW
};

const pb_obj_enum_member_t pb_Color_ORANGE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_ORANGE,
    .value = PBIO_COLOR_ORANGE
};

const pb_obj_enum_member_t pb_Color_RED_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_RED,
    .value = PBIO_COLOR_RED
};

const pb_obj_enum_member_t pb_Color_WHITE_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_WHITE,
    .value = PBIO_COLOR_WHITE
};

const pb_obj_enum_member_t pb_Color_BROWN_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_BROWN,
    .value = PBIO_COLOR_BROWN
};

const pb_obj_enum_member_t pb_Color_GRAY_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_GRAY,
    .value = PBIO_COLOR_GRAY
};

const pb_obj_enum_member_t pb_Color_CYAN_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_CYAN,
    .value = PBIO_COLOR_CYAN
};

const pb_obj_enum_member_t pb_Color_MAGENTA_obj = {
    {&pb_enum_type_Color},
    .name = MP_QSTR_MAGENTA,
    .value = PBIO_COLOR_MAGENTA
};

STATIC const mp_rom_map_elem_t pb_enum_Color_table[] = {
    { MP_ROM_QSTR(MP_QSTR_BLACK),   MP_ROM_PTR(&pb_Color_BLACK_obj)  },
    { MP_ROM_QSTR(MP_QSTR_PURPLE),  MP_ROM_PTR(&pb_Color_PURPLE_obj) },
    { MP_ROM_QSTR(MP_QSTR_BLUE),    MP_ROM_PTR(&pb_Color_BLUE_obj)   },
    { MP_ROM_QSTR(MP_QSTR_GREEN),   MP_ROM_PTR(&pb_Color_GREEN_obj)  },
    { MP_ROM_QSTR(MP_QSTR_YELLOW),  MP_ROM_PTR(&pb_Color_YELLOW_obj) },
    { MP_ROM_QSTR(MP_QSTR_ORANGE),  MP_ROM_PTR(&pb_Color_ORANGE_obj) },
    { MP_ROM_QSTR(MP_QSTR_RED),     MP_ROM_PTR(&pb_Color_RED_obj)    },
    { MP_ROM_QSTR(MP_QSTR_WHITE),   MP_ROM_PTR(&pb_Color_WHITE_obj)  },
    { MP_ROM_QSTR(MP_QSTR_BROWN),   MP_ROM_PTR(&pb_Color_BROWN_obj)  },
    { MP_ROM_QSTR(MP_QSTR_GRAY),    MP_ROM_PTR(&pb_Color_GRAY_obj)   },
    { MP_ROM_QSTR(MP_QSTR_CYAN),    MP_ROM_PTR(&pb_Color_CYAN_obj)   },
    { MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_ROM_PTR(&pb_Color_MAGENTA_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_enum_type_Color_locals_dict, pb_enum_Color_table);

const mp_obj_type_t pb_enum_type_Color = {
    { &mp_type_type },
    .name = MP_QSTR_Color,
    .print = pb_type_enum_print,
    .unary_op = mp_generic_unary_op,
    .locals_dict = (mp_obj_dict_t *)&(pb_enum_type_Color_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS
