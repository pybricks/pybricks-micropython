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

mp_obj_t light_Light_obj_make_new(pbio_lightdev_t dev);

#endif // _PYBRICKS_EXTMOD_MODLIGHT_H_
