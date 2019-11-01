// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_EXTMOD_MODLIGHT_H_
#define _PYBRICKS_EXTMOD_MODLIGHT_H_

#include <pbio/ev3device.h>

// TODO: Ultimately, lights should be redesigned and moved to pbio
typedef struct _pbio_lightdev_t {
    pbio_iodev_type_id_t id;
    pbio_iodev_t *pupiodev;
    pbio_ev3iodev_t *ev3iodev;
} pbio_lightdev_t;

#include "py/obj.h"

const mp_obj_type_t light_Light_type;
const mp_obj_type_t light_ColorLight_type;

mp_obj_t light_Light_obj_make_new(pbio_lightdev_t dev, const mp_obj_type_t *type);

#endif // _PYBRICKS_EXTMOD_MODLIGHT_H_
