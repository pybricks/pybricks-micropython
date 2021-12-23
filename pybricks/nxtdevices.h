// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_NXTDEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_NXTDEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/obj.h"
#include <pybricks/util_mp/pb_obj_helper.h>

extern const mp_obj_module_t pb_module_nxtdevices;

extern const pb_obj_with_attr_type_t pb_type_nxtdevices_ColorSensor;
extern const mp_obj_type_t pb_type_nxtdevices_EnergyMeter;
extern const mp_obj_type_t pb_type_nxtdevices_LightSensor;
extern const mp_obj_type_t pb_type_nxtdevices_SoundSensor;
extern const mp_obj_type_t pb_type_nxtdevices_TemperatureSensor;
extern const mp_obj_type_t pb_type_nxtdevices_TouchSensor;
extern const mp_obj_type_t pb_type_nxtdevices_UltrasonicSensor;

int32_t analog_scale(int32_t mvolts, int32_t mvolts_min, int32_t mvolts_max, bool invert);

#endif // PYBRICKS_PY_NXTDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_NXTDEVICES_H
