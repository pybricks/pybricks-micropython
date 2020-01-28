// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_EXTMOD_MODBUILTINS_H_
#define _PYBRICKS_EXTMOD_MODBUILTINS_H_

#include <pbio/control.h>

#include "py/obj.h"

const mp_obj_type_t builtins_Control_type;
mp_obj_t builtins_Control_obj_make_new(pbio_control_t *control);

#endif // _PYBRICKS_EXTMOD_MODBUILTINS_H_
