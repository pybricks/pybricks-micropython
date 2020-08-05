// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/obj.h"

const mp_obj_module_t pb_module_pupdevices;

const mp_obj_type_t pb_type_pupdevices_ColorDistanceSensor;
const mp_obj_type_t pb_type_pupdevices_ColorSensor;
const mp_obj_type_t pb_type_pupdevices_ForceSensor;
const mp_obj_type_t pb_type_pupdevices_InfraredSensor;
const mp_obj_type_t pb_type_pupdevices_Light;
const mp_obj_type_t pb_type_pupdevices_TiltSensor;
const mp_obj_type_t pb_type_pupdevices_UltrasonicSensor;

#endif // PYBRICKS_PY_PUPDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
