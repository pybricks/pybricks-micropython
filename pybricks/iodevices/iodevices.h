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

extern const mp_obj_type_t pb_type_i2c_device;
/**
 * Given a completed I2C operation, maps the resulting read buffer to an object
 * of a desired form. For example, it could map two bytes to a single floating
 * point value representing temperature.
 *
 * @param [in]  data   The data read.
 * @param [in]  len    The data length.
 */
typedef mp_obj_t (*pb_type_i2c_device_return_map_t)(const uint8_t *data, size_t len);

mp_obj_t pb_type_i2c_device_make_new(mp_obj_t port_in, mp_obj_t address_in, bool custom, bool powered, bool nxt_quirk);
mp_obj_t pb_type_i2c_device_start_operation(mp_obj_t i2c_device_obj, const uint8_t *write_data, size_t write_len, size_t read_len, pb_type_i2c_device_return_map_t return_map);

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
