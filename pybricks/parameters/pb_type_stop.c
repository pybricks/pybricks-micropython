// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/control.h>

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_type_enum.h>

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

const pb_obj_enum_member_t pb_Stop_NONE_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_NONE,
    .value = PBIO_ACTUATION_CONTINUE
};

STATIC const mp_rom_map_elem_t pb_enum_Stop_table[] = {
    { MP_ROM_QSTR(MP_QSTR_COAST),   MP_ROM_PTR(&pb_Stop_COAST_obj) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE),   MP_ROM_PTR(&pb_Stop_BRAKE_obj) },
    { MP_ROM_QSTR(MP_QSTR_HOLD),    MP_ROM_PTR(&pb_Stop_HOLD_obj)  },
    { MP_ROM_QSTR(MP_QSTR_NONE),    MP_ROM_PTR(&pb_Stop_NONE_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_enum_type_Stop_locals_dict, pb_enum_Stop_table);

const mp_obj_type_t pb_enum_type_Stop = {
    { &mp_type_type },
    .name = MP_QSTR_Stop,
    .print = pb_type_enum_print,
    .unary_op = mp_generic_unary_op,
    .locals_dict = (mp_obj_dict_t *)&(pb_enum_type_Stop_locals_dict),
};
#endif // PYBRICKS_PY_PARAMETERS
