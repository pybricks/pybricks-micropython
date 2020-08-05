// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_EV3DEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_EV3DEVICES_H

#include "py/mpconfig.h"
#include "py/obj.h"

#if PYBRICKS_PY_EV3DEVICES

const mp_obj_module_t pb_module_ev3devices;

const mp_obj_type_t pb_type_ev3devices_ColorSensor;
const mp_obj_type_t pb_type_ev3devices_InfraredSensor;
const mp_obj_type_t pb_type_ev3devices_GyroSensor;
const mp_obj_type_t pb_type_ev3devices_TouchSensor;
const mp_obj_type_t pb_type_ev3devices_UltrasonicSensor;

#endif // PYBRICKS_PY_EV3DEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_EV3DEVICES_H
