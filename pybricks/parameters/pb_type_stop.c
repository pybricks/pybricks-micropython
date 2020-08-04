// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/control.h>

#include "parameters/parameters.h"
#include "util_mp/pb_type_enum.h"

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

#endif // PYBRICKS_PY_PARAMETERS
