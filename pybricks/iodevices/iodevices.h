// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H

#include "py/mpconfig.h"
#include "py/obj.h"

#if PYBRICKS_PY_IODEVICES

const mp_obj_module_t pb_module_iodevices;

const mp_obj_type_t pb_type_iodevices_LUMPDevice;

#if PYBRICKS_PY_EV3DEVICES

const mp_obj_type_t pb_type_iodevices_AnalogSensor;
const mp_obj_type_t pb_type_iodevices_Ev3devSensor;
const mp_obj_type_t pb_type_iodevices_I2CDevice;
const mp_obj_type_t pb_type_iodevices_UARTDevice;

#endif // PYBRICKS_PY_EV3DEVICES

#endif // PYBRICKS_PY_IODEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
