// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PYBRICKS_EXTMOD_MODBUILTINS_H_
#define _PYBRICKS_EXTMOD_MODBUILTINS_H_

#include <pbio/control.h>

#include "pbdevice.h"

#include "py/obj.h"

const mp_obj_type_t builtins_Light_type;
const mp_obj_type_t builtins_ColorLight_type;

mp_obj_t builtins_Light_obj_make_new(pbdevice_t *pbdev, const mp_obj_type_t *type);

const mp_obj_type_t builtins_Control_type;
mp_obj_t builtins_Control_obj_make_new(pbio_control_t *control);

#endif // _PYBRICKS_EXTMOD_MODBUILTINS_H_
