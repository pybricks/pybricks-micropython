// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H

#include "py/mpconfig.h"

#include <pbio/port.h>

#if PYBRICKS_PY_IODEVICES

#include "py/obj.h"

#include <pbio/port.h>

#include <pybricks/tools/pb_type_async.h>

#if PYBRICKS_PY_IODEVICES_PUP_DEVICE
extern const mp_obj_type_t pb_type_iodevices_PUPDevice;
#endif

#if PYBRICKS_PY_IODEVICES_UART_DEVICE
extern const mp_obj_type_t pb_type_uart_device;
#endif

#if PYBRICKS_PY_IODEVICES_LWP3_DEVICE
extern const mp_obj_type_t pb_type_lwp3device;
#endif

#if PYBRICKS_PY_IODEVICES_BLUETOOTH_CLASSIC_GAMEPAD
extern const mp_obj_type_t pb_type_iodevices_BluetoothClassicGamepad;
#endif

#if PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER
extern const mp_obj_type_t pb_type_iodevices_XboxController;
#endif

#if PYBRICKS_PY_IODEVICES_ANALOG_SENSOR
extern const mp_obj_type_t pb_type_iodevices_AnalogSensor;
#endif

#if PYBRICKS_PY_IODEVICES_I2C_DEVICE
extern const mp_obj_type_t pb_type_i2c_device;
#endif

pbio_port_power_requirements_t pb_module_iodevices_get_requested_power_pin(mp_obj_t power_pin_in);

#endif // PYBRICKS_PY_IODEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
