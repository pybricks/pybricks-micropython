// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_NXTDEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_NXTDEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/obj.h"

const mp_obj_module_t pb_module_nxtdevices;

const mp_obj_type_t pb_type_nxtdevices_ColorSensor;
const mp_obj_type_t pb_type_nxtdevices_EnergyMeter;
const mp_obj_type_t pb_type_nxtdevices_LightSensor;
const mp_obj_type_t pb_type_nxtdevices_SoundSensor;
const mp_obj_type_t pb_type_nxtdevices_TemperatureSensor;
const mp_obj_type_t pb_type_nxtdevices_TouchSensor;
const mp_obj_type_t pb_type_nxtdevices_UltrasonicSensor;

int32_t analog_scale(int32_t mvolts, int32_t mvolts_min, int32_t mvolts_max, bool invert);

#endif // PYBRICKS_PY_NXTDEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_NXTDEVICES_H
