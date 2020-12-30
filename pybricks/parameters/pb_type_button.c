// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#if PYBRICKS_PY_PARAMETERS_BUTTON

#include <pbio/button.h>

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_type_enum.h>

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

const pb_obj_enum_member_t pb_Button_LEFT_PLUS_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_LEFT_PLUS,
    .value = PBIO_BUTTON_LEFT_UP
};


const pb_obj_enum_member_t pb_Button_LEFT_DOWN_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_LEFT_DOWN,
    .value = PBIO_BUTTON_LEFT_DOWN
};

const pb_obj_enum_member_t pb_Button_LEFT_MINUS_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_LEFT_MINUS,
    .value = PBIO_BUTTON_LEFT_DOWN
};

const pb_obj_enum_member_t pb_Button_RIGHT_UP_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_RIGHT_UP,
    .value = PBIO_BUTTON_RIGHT_UP
};

const pb_obj_enum_member_t pb_Button_RIGHT_PLUS_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_RIGHT_PLUS,
    .value = PBIO_BUTTON_RIGHT_UP
};

const pb_obj_enum_member_t pb_Button_RIGHT_DOWN_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_RIGHT_DOWN,
    .value = PBIO_BUTTON_RIGHT_DOWN
};

const pb_obj_enum_member_t pb_Button_RIGHT_MINUS_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_RIGHT_MINUS,
    .value = PBIO_BUTTON_RIGHT_DOWN
};

const pb_obj_enum_member_t pb_Button_BEACON_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_BEACON,
    .value = PBIO_BUTTON_UP
};

const pb_obj_enum_member_t pb_Button_BLUETOOTH_obj = {
    {&pb_enum_type_Button},
    .name = MP_QSTR_BLUETOOTH,
    .value = PBIO_BUTTON_RIGHT_UP
};

STATIC const mp_rom_map_elem_t pb_enum_Button_table[] = {
    { MP_ROM_QSTR(MP_QSTR_UP),         MP_ROM_PTR(&pb_Button_UP_obj)        },
    { MP_ROM_QSTR(MP_QSTR_DOWN),       MP_ROM_PTR(&pb_Button_DOWN_obj)      },
    { MP_ROM_QSTR(MP_QSTR_LEFT),       MP_ROM_PTR(&pb_Button_LEFT_obj)      },
    { MP_ROM_QSTR(MP_QSTR_RIGHT),      MP_ROM_PTR(&pb_Button_RIGHT_obj)     },
    { MP_ROM_QSTR(MP_QSTR_CENTER),     MP_ROM_PTR(&pb_Button_CENTER_obj)    },
    { MP_ROM_QSTR(MP_QSTR_LEFT_UP),    MP_ROM_PTR(&pb_Button_LEFT_UP_obj)   },
    { MP_ROM_QSTR(MP_QSTR_LEFT_PLUS),  MP_ROM_PTR(&pb_Button_LEFT_PLUS_obj) },
    { MP_ROM_QSTR(MP_QSTR_LEFT_DOWN),  MP_ROM_PTR(&pb_Button_LEFT_DOWN_obj) },
    { MP_ROM_QSTR(MP_QSTR_LEFT_MINUS), MP_ROM_PTR(&pb_Button_LEFT_MINUS_obj)},
    { MP_ROM_QSTR(MP_QSTR_RIGHT_UP),   MP_ROM_PTR(&pb_Button_RIGHT_UP_obj)  },
    { MP_ROM_QSTR(MP_QSTR_RIGHT_PLUS), MP_ROM_PTR(&pb_Button_RIGHT_PLUS_obj)},
    { MP_ROM_QSTR(MP_QSTR_RIGHT_DOWN), MP_ROM_PTR(&pb_Button_RIGHT_DOWN_obj)},
    { MP_ROM_QSTR(MP_QSTR_RIGHT_MINUS),MP_ROM_PTR(&pb_Button_RIGHT_MINUS_obj)},
    { MP_ROM_QSTR(MP_QSTR_BEACON),     MP_ROM_PTR(&pb_Button_BEACON_obj)    },
    { MP_ROM_QSTR(MP_QSTR_BLUETOOTH),  MP_ROM_PTR(&pb_Button_BLUETOOTH_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_enum_type_Button_locals_dict, pb_enum_Button_table);

const mp_obj_type_t pb_enum_type_Button = {
    { &mp_type_type },
    .name = MP_QSTR_Button,
    .print = pb_type_enum_print,
    .unary_op = mp_generic_unary_op,
    .locals_dict = (mp_obj_dict_t *)&(pb_enum_type_Button_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS_BUTTON

#endif // PYBRICKS_PY_PARAMETERS
