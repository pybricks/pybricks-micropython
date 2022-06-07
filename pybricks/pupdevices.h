// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2022 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/obj.h"

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>

extern const mp_obj_module_t pb_module_pupdevices;

extern const pb_obj_with_attr_type_t pb_type_pupdevices_ColorDistanceSensor;
extern const mp_obj_type_t pb_type_pupdevices_ColorLightMatrix;
extern const pb_obj_with_attr_type_t pb_type_pupdevices_ColorSensor;
extern const mp_obj_type_t pb_type_pupdevices_ForceSensor;
extern const mp_obj_type_t pb_type_pupdevices_InfraredSensor;
extern const mp_obj_type_t pb_type_pupdevices_Light;
extern const mp_obj_type_t pb_type_pupdevices_PFMotor;
extern const pb_obj_with_attr_type_t pb_type_pupdevices_Remote;
extern const mp_obj_type_t pb_type_pupdevices_TiltSensor;
extern const pb_obj_with_attr_type_t pb_type_pupdevices_UltrasonicSensor;

pb_device_t *pupdevices_ColorDistanceSensor__get_device(mp_obj_t obj);

#endif // PYBRICKS_PY_PUPDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_PUPDEVICES_H
