// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
#define PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS

#include <math.h>

#include "py/obj.h"

#include "pybricks/util_mp/pb_obj_helper.h"

extern const pb_obj_with_attr_type_t pb_type_drivebase;
extern const pb_obj_with_attr_type_t pb_type_spikebase;

#endif // PYBRICKS_PY_ROBOTICS

#endif // PYBRICKS_INCLUDED_PYBRICKS_ROBOTICS_H
