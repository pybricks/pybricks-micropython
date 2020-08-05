// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/dcmotor.h>

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_type_enum.h>

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
STATIC MP_DEFINE_CONST_DICT(pb_enum_type_Direction_locals_dict, pb_enum_Direction_table);

const mp_obj_type_t pb_enum_type_Direction = {
    { &mp_type_type },
    .name = MP_QSTR_Direction,
    .print = pb_type_enum_print,
    .unary_op = mp_generic_unary_op,
    .locals_dict = (mp_obj_dict_t *)&(pb_enum_type_Direction_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS
