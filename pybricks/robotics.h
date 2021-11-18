// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
#define PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS

#include <math.h>

#include "py/obj.h"

extern const mp_obj_type_t pb_type_spikebase;

extern const mp_obj_type_t pb_type_drivebase;

extern const mp_obj_module_t pb_module_robotics;

#endif // PYBRICKS_PY_ROBOTICS

#endif // PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
