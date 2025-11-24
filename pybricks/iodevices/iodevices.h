// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
#define PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include "py/obj.h"

#include <pybricks/tools/pb_type_async.h>

#if PYBRICKS_PY_IODEVICES_PUP_DEVICE
extern const mp_obj_type_t pb_type_iodevices_PUPDevice;
#endif

#if PYBRICKS_PY_IODEVICES_UART_DEVICE
extern const mp_obj_type_t pb_type_uart_device;
#endif

#if PYBRICKS_PY_IODEVICES_LWP3_DEVICE
extern const mp_obj_type_t pb_type_iodevices_LWP3Device;
#endif

#if PYBRICKS_PY_IODEVICES_XBOX_CONTROLLER
extern const mp_obj_type_t pb_type_iodevices_XboxController;
#endif

#if PYBRICKS_PY_IODEVICES_ANALOG_SENSOR
extern const mp_obj_type_t pb_type_iodevices_AnalogSensor;
#endif

#if PYBRICKS_PY_IODEVICES_I2C_DEVICE

extern const mp_obj_type_t pb_type_i2c_device;
/**
 * Given a completed I2C operation, maps the resulting read buffer to an object
 * of a desired form. For example, it could map two bytes to a single floating
 * point value representing temperature.
 *
 * @param [in]  sensor_obj Instance of sensor that owns this device or MP_OBJ_NULL for the standalone I2CDevice class.
 * @param [in]  data   The data read.
 * @param [in]  len    The data length.
 * @return             Resulting object to return to user.
 */
typedef mp_obj_t (*pb_type_i2c_device_return_map_t)(mp_obj_t sensor_obj, const uint8_t *data, size_t len);

mp_obj_t pb_type_i2c_device_make_new(mp_obj_t sensor_obj, mp_obj_t port_in, uint8_t address, bool custom, bool powered, bool nxt_quirk);
mp_obj_t pb_type_i2c_device_start_operation(mp_obj_t i2c_device_obj, const uint8_t *write_data, size_t write_len, size_t read_len, pb_type_i2c_device_return_map_t return_map);
void pb_type_i2c_device_assert_string_at_register(mp_obj_t i2c_device_obj, uint8_t reg, const char *string);

#endif

#endif // PYBRICKS_PY_IODEVICES

#endif // PYBRICKS_INCLUDED_PYBRICKS_IODEVICES_H
