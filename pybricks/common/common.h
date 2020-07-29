// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
#define PYBRICKS_INCLUDED_PYBRICKS_COMMON_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include <pbio/control.h>

#include "py/obj.h"

const mp_obj_type_t pb_type_Control;
mp_obj_t common_Control_obj_make_new(pbio_control_t *control);

#endif // PYBRICKS_PY_COMMON

#endif // PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
