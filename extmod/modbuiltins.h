// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_EXTMOD_MODBUILTINS_H_
#define _PYBRICKS_EXTMOD_MODBUILTINS_H_

#include "py/obj.h"

const mp_obj_type_t builtins_Speaker_type;
mp_obj_t builtins_Speaker_obj_make_new(uint8_t volume);

#endif // _PYBRICKS_EXTMOD_MODBUILTINS_H_
