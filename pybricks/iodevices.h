// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include "py/obj.h"

#include <pybricks/util_pb/pb_device.h>

extern const mp_obj_module_t pb_module_iodevices;

#if PYBRICKS_PY_PUPDEVICES

extern const mp_obj_type_t pb_type_iodevices_LWP3Device;
extern const mp_obj_type_t pb_type_iodevices_PUPDevice;

#endif // PYBRICKS_PY_PUPDEVICES

#if PYBRICKS_PY_EV3DEVICES

extern const mp_obj_type_t pb_type_iodevices_LUMPDevice;
extern const mp_obj_type_t pb_type_iodevices_AnalogSensor;
extern const mp_obj_type_t pb_type_iodevices_Ev3devSensor;
extern const mp_obj_type_t pb_type_iodevices_I2CDevice;
extern const mp_obj_type_t pb_type_iodevices_UARTDevice;

#endif // PYBRICKS_PY_EV3DEVICES

#endif // PYBRICKS_PY_IODEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
