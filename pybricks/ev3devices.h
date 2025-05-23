// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_EV3DEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_EV3DEVICES_H

#include "py/mpconfig.h"

#include "py/obj.h"

#if PYBRICKS_PY_EV3DEVICES

extern const mp_obj_type_t pb_type_ev3devices_TouchSensor;
extern const mp_obj_type_t pb_type_ev3devices_ColorSensor;

#endif // PYBRICKS_PY_EV3DEVICES

#if PYBRICKS_PY_EV3DEVDEVICES

extern const mp_obj_type_t pb_type_ev3devices_InfraredSensor;
extern const mp_obj_type_t pb_type_ev3devices_GyroSensor;
extern const mp_obj_type_t pb_type_ev3devices_UltrasonicSensor;

#endif // PYBRICKS_PY_EV3DEVDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_EV3DEVICES_H
