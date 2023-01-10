// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/obj.h"

#include <pybricks/util_pb/pb_device.h>

extern const mp_obj_type_t pb_type_pupdevices_ColorDistanceSensor;
extern const mp_obj_type_t pb_type_pupdevices_ColorLightMatrix;
extern const mp_obj_type_t pb_type_pupdevices_ColorSensor;
extern const mp_obj_type_t pb_type_pupdevices_ForceSensor;
extern const mp_obj_type_t pb_type_pupdevices_InfraredSensor;
extern const mp_obj_type_t pb_type_pupdevices_Light;
extern const mp_obj_type_t pb_type_pupdevices_PFMotor;
extern const mp_obj_type_t pb_type_pupdevices_Remote;
extern const mp_obj_type_t pb_type_pupdevices_TiltSensor;
extern const mp_obj_type_t pb_type_pupdevices_UltrasonicSensor;

pb_device_t *pupdevices_ColorDistanceSensor__get_device(mp_obj_t obj);
void pb_type_Remote_cleanup(void);

#endif // PYBRICKS_PY_PUPDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
