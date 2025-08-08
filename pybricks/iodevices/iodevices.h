// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include "py/obj.h"

extern const mp_obj_type_t pb_type_iodevices_PUPDevice;
extern const mp_obj_type_t pb_type_uart_device;

#if PYBRICKS_PY_IODEVICES_I2CDEVICE
extern const mp_obj_type_t pb_type_iodevices_I2CDevice;
#endif

#if PYBRICKS_PY_PUPDEVICES

extern const mp_obj_type_t pb_type_iodevices_LWP3Device;
extern const mp_obj_type_t pb_type_iodevices_XboxController;

#endif // PYBRICKS_PY_PUPDEVICES

#if PYBRICKS_PY_EV3DEVICES

extern const mp_obj_type_t pb_type_iodevices_AnalogSensor;
extern const mp_obj_type_t pb_type_iodevices_Ev3devSensor;

#endif // PYBRICKS_PY_EV3DEVICES

#endif // PYBRICKS_PY_IODEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
