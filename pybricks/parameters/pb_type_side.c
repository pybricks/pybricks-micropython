// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/light_matrix.h>

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_type_enum.h>

const pb_obj_enum_member_t pb_Side_BACK_obj = {
    {&pb_enum_type_Side},
    .name = MP_QSTR_BACK,
    .value = PBIO_ORIENTATION_SIDE_BACK
};

const pb_obj_enum_member_t pb_Side_BOTTOM_obj = {
    {&pb_enum_type_Side},
    .name = MP_QSTR_BOTTOM,
    .value = PBIO_ORIENTATION_SIDE_BOTTOM
};

const pb_obj_enum_member_t pb_Side_FRONT_obj = {
    {&pb_enum_type_Side},
    .name = MP_QSTR_FRONT,
    .value = PBIO_ORIENTATION_SIDE_FRONT
};

const pb_obj_enum_member_t pb_Side_LEFT_obj = {
    {&pb_enum_type_Side},
    .name = MP_QSTR_LEFT,
    .value = PBIO_ORIENTATION_SIDE_LEFT
};

const pb_obj_enum_member_t pb_Side_RIGHT_obj = {
    {&pb_enum_type_Side},
    .name = MP_QSTR_RIGHT,
    .value = PBIO_ORIENTATION_SIDE_RIGHT
};

const pb_obj_enum_member_t pb_Side_TOP_obj = {
    {&pb_enum_type_Side},
    .name = MP_QSTR_TOP,
    .value = PBIO_ORIENTATION_SIDE_TOP
};

STATIC const mp_rom_map_elem_t pb_enum_Side_table[] = {
    { MP_ROM_QSTR(MP_QSTR_BACK),   MP_ROM_PTR(&pb_Side_BACK_obj)  },
    { MP_ROM_QSTR(MP_QSTR_BOTTOM), MP_ROM_PTR(&pb_Side_BOTTOM_obj)},
    { MP_ROM_QSTR(MP_QSTR_FRONT),  MP_ROM_PTR(&pb_Side_FRONT_obj) },
    { MP_ROM_QSTR(MP_QSTR_LEFT),   MP_ROM_PTR(&pb_Side_LEFT_obj)  },
    { MP_ROM_QSTR(MP_QSTR_RIGHT),  MP_ROM_PTR(&pb_Side_RIGHT_obj) },
    { MP_ROM_QSTR(MP_QSTR_TOP),    MP_ROM_PTR(&pb_Side_TOP_obj)   },
};
STATIC MP_DEFINE_CONST_DICT(pb_enum_type_Side_locals_dict, pb_enum_Side_table);

const mp_obj_type_t pb_enum_type_Side = {
    { &mp_type_type },
    .name = MP_QSTR_Side,
    .print = pb_type_enum_print,
    .unary_op = mp_generic_unary_op,
    .locals_dict = (mp_obj_dict_t *)&(pb_enum_type_Side_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS
