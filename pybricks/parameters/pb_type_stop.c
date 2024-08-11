// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/control.h>

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_type_enum.h>

const pb_obj_enum_member_t pb_Stop_COAST_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_COAST,
    .value = PBIO_CONTROL_ON_COMPLETION_COAST
};

const pb_obj_enum_member_t pb_Stop_COAST_SMART_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_COAST_SMART,
    .value = PBIO_CONTROL_ON_COMPLETION_COAST_SMART,
};

const pb_obj_enum_member_t pb_Stop_BRAKE_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_BRAKE,
    .value = PBIO_CONTROL_ON_COMPLETION_BRAKE
};

const pb_obj_enum_member_t pb_Stop_BRAKE_SMART_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_BRAKE_SMART,
    .value = PBIO_CONTROL_ON_COMPLETION_BRAKE_SMART,
};

const pb_obj_enum_member_t pb_Stop_HOLD_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_HOLD,
    .value = PBIO_CONTROL_ON_COMPLETION_HOLD
};

const pb_obj_enum_member_t pb_Stop_NONE_obj = {
    {&pb_enum_type_Stop},
    .name = MP_QSTR_NONE,
    .value = PBIO_CONTROL_ON_COMPLETION_CONTINUE
};

static const mp_rom_map_elem_t pb_enum_Stop_table[] = {
    { MP_ROM_QSTR(MP_QSTR_COAST),       MP_ROM_PTR(&pb_Stop_COAST_obj) },
    { MP_ROM_QSTR(MP_QSTR_COAST_SMART), MP_ROM_PTR(&pb_Stop_COAST_SMART_obj) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE),       MP_ROM_PTR(&pb_Stop_BRAKE_obj) },
    { MP_ROM_QSTR(MP_QSTR_BRAKE_SMART), MP_ROM_PTR(&pb_Stop_BRAKE_SMART_obj) },
    { MP_ROM_QSTR(MP_QSTR_HOLD),        MP_ROM_PTR(&pb_Stop_HOLD_obj)  },
    { MP_ROM_QSTR(MP_QSTR_NONE),        MP_ROM_PTR(&pb_Stop_NONE_obj)},
};
static MP_DEFINE_CONST_DICT(pb_enum_type_Stop_locals_dict, pb_enum_Stop_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_enum_type_Stop,
    MP_QSTR_Stop,
    MP_TYPE_FLAG_NONE,
    print, pb_type_enum_print,
    unary_op, mp_generic_unary_op,
    locals_dict, &(pb_enum_type_Stop_locals_dict));

#endif // PYBRICKS_PY_PARAMETERS
