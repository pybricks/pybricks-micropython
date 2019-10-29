// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_EXTMOD_MODLIGHT_H_
#define _PYBRICKS_EXTMOD_MODLIGHT_H_

#include <pbio/ev3device.h>

#include "py/obj.h"

#if PBDRV_CONFIG_HUB_ID == PBIO_IODEV_TYPE_ID_EV3_BRICK // FIXME: Don't use hub name here; make compatible with PUPDEVICES
mp_obj_t ev3devices_Light_obj_make_new(pbio_ev3iodev_t *iodev);
#endif

#endif // _PYBRICKS_EXTMOD_MODLIGHT_H_
