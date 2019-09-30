// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#ifndef _PYBRICKS_EXTMOD_MODLOGGER_H_
#define _PYBRICKS_EXTMOD_MODLOGGER_H_

#include <pbio/servo.h>

#include "py/obj.h"

mp_obj_t logger_obj_make_new(pbio_servo_t *srv);

#endif // _PYBRICKS_EXTMOD_MODLOGGER_H_
