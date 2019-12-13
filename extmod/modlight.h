// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_EXTMOD_MODLIGHT_H_
#define _PYBRICKS_EXTMOD_MODLIGHT_H_

#include <pbio/ev3device.h>

#include "pblight.h"

#include "py/obj.h"

const mp_obj_type_t light_Light_type;
const mp_obj_type_t light_ColorLight_type;

mp_obj_t light_Light_obj_make_new(pbio_lightdev_t dev, const mp_obj_type_t *type);

#endif // _PYBRICKS_EXTMOD_MODLIGHT_H_
