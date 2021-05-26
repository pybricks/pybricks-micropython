// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_HUBS_H
#define PYBRICKS_INCLUDED_PYBRICKS_HUBS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS

#include "py/obj.h"

MP_DECLARE_CONST_FUN_OBJ_2(pb_hubs_Hub_reset_obj);
MP_DECLARE_CONST_FUN_OBJ_1(pb_hubs_Hub_reset_reason_obj);

extern const mp_obj_type_t pb_type_ThisHub;

extern const mp_obj_module_t pb_module_hubs;

#endif // PYBRICKS_PY_HUBS

#endif // PYBRICKS_INCLUDED_PYBRICKS_HUBS_H
