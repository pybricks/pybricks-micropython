// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/dcmotor.h>

#include "parameters/parameters.h"
#include "util_mp/pb_type_enum.h"

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

#endif // PYBRICKS_PY_PARAMETERS
