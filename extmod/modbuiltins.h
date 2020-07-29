// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef _PYBRICKS_EXTMOD_MODBUILTINS_H_
#define _PYBRICKS_EXTMOD_MODBUILTINS_H_

#include <pbio/control.h>

#include "pbdevice.h"

#include "py/obj.h"

mp_obj_t builtins_ColorLight_obj_make_new(pbdevice_t *pbdev);
mp_obj_t builtins_LightArray_obj_make_new(pbdevice_t *pbdev, uint8_t light_mode, uint8_t number_of_lights);

#endif // _PYBRICKS_EXTMOD_MODBUILTINS_H_
