// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
#define PYBRICKS_INCLUDED_PYBRICKS_COMMON_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include <stdint.h>

#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/light.h>
#include <pbdrv/legodev.h>

#include "py/obj.h"

#include <pybricks/util_mp/pb_obj_helper.h>

#include <pybricks/parameters.h>
#include <pybricks/parameters/pb_type_button.h>
#include <pybricks/pupdevices.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_awaitable.h>
#include <pybricks/common/pb_type_device.h>

void pb_package_pybricks_init(bool import_all);
void pb_package_pybricks_deinit(void);

#if PYBRICKS_PY_COMMON_BLE
mp_obj_t pb_type_BLE_new(mp_obj_t broadcast_channel_in, mp_obj_t observe_channels_in);
void pb_type_ble_start_cleanup(void);
#endif

#if PYBRICKS_PY_COMMON_CHARGER

extern const mp_obj_type_t pb_type_Charger;
mp_obj_t pb_type_Charger_obj_new(void);

#endif // PYBRICKS_PY_COMMON_CHARGER

/**
 * Device-specific callback for controlling a color light.
 * @param [in]  context     The instance-specific context.
 * @param [in]  hsv         The HSV color for the light.
 * @return                  None or awaitable.
 */
typedef mp_obj_t (*pb_type_ColorLight_on_t)(void *context, const pbio_color_hsv_t *hsv);

// pybricks._common.ColorLight()
mp_obj_t pb_type_ColorLight_external_obj_new(void *context, pb_type_ColorLight_on_t on);
mp_obj_t common_ColorLight_internal_obj_new(pbio_color_light_t *light);

#if PYBRICKS_PY_COMMON_LIGHT_ARRAY
// pybricks._common.LightArray()
mp_obj_t common_LightArray_obj_make_new(pb_type_device_obj_base_t *sensor, uint8_t light_mode, uint8_t number_of_lights);
#endif

#if PYBRICKS_PY_COMMON_LIGHT_MATRIX
#include <pbio/light_matrix.h>
// pybricks._common.LightMatrix()
extern const uint8_t pb_digits_5x2[10][5];
extern const uint8_t pb_font_5x5[95][5];
mp_obj_t pb_type_LightMatrix_obj_new(pbio_light_matrix_t *light_matrix);
void pb_type_LightMatrix_display_number(pbio_light_matrix_t *light_matrix, mp_obj_t number_in);
void pb_type_LightMatrix_display_char(pbio_light_matrix_t *light_matrix, mp_obj_t char_in);
#endif

#if PYBRICKS_PY_COMMON_KEYPAD
// pybricks._common.KeyPad()
mp_obj_t pb_type_Keypad_obj_new(pb_type_button_get_pressed_t get_pressed);
#endif

// pybricks._common.Battery()
extern const mp_obj_module_t pb_module_battery;


#if PYBRICKS_PY_COMMON_MOTORS

#include <pbio/control.h>
#include <pbio/port.h>
#include <pbio/servo.h>

mp_obj_t make_acceleration_return_value(int32_t acceleration, int32_t deceleration);
void unpack_acceleration_value(mp_obj_t accel_in, int32_t *acceleration, int32_t *deceleration);

#if PYBRICKS_PY_COMMON_CONTROL
// pybricks._common.Control()
extern const mp_obj_type_t pb_type_Control;
mp_obj_t pb_type_Control_obj_make_new(pbio_control_t *control);
#endif

#if PYBRICKS_PY_COMMON_MOTOR_MODEL
// pybricks._common.MotorModel()
extern const mp_obj_type_t pb_type_MotorModel;
mp_obj_t pb_type_MotorModel_obj_make_new(pbio_observer_t *observer);
#endif

#if PYBRICKS_PY_COMMON_LOGGER
// pybricks._common.Logger()
mp_obj_t common_Logger_obj_make_new(pbio_log_t *log, uint8_t num_values);
#endif

// pybricks.common.DCMotor and pybricks.common.Motor
typedef struct {
    pb_type_device_obj_base_t device_base;
    pbio_servo_t *srv;
    pbio_port_id_t port;
    #if PYBRICKS_PY_COMMON_MOTOR_MODEL
    mp_obj_t model;
    #endif
    #if PYBRICKS_PY_COMMON_CONTROL
    mp_obj_t control;
    #endif
    #if PYBRICKS_PY_COMMON_LOGGER
    mp_obj_t logger;
    #endif
} pb_type_Motor_obj_t;

extern const mp_obj_type_t pb_type_Motor;
extern const mp_obj_type_t pb_type_DCMotor;

pbio_servo_t *pb_type_motor_get_servo(mp_obj_t motor_in);

#endif // PYBRICKS_PY_COMMON_MOTORS

#if PYBRICKS_PY_COMMON_SPEAKER

extern const mp_obj_type_t pb_type_Speaker;

#endif // PYBRICKS_PY_COMMON_SPEAKER

#if PYBRICKS_PY_COMMON_IMU

mp_obj_t pb_type_IMU_obj_new(mp_obj_t hub_in, mp_obj_t top_side_axis, mp_obj_t front_side_axis);

#endif // PYBRICKS_PY_COMMON_IMU


#if PYBRICKS_PY_COMMON_SYSTEM

extern const mp_obj_module_t pb_type_System;

#endif // PYBRICKS_PY_COMMON_SYSTEM

#endif // PYBRICKS_PY_COMMON

#endif // PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
