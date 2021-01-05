// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
#define PYBRICKS_INCLUDED_PYBRICKS_COMMON_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include <stdint.h>

#include <pbio/light.h>

#include "py/obj.h"

#include <pybricks/util_pb/pb_device.h>

#include <pybricks/parameters.h>

// pybricks._common.ColorLight()
mp_obj_t common_ColorLight_external_obj_make_new(pb_device_t *pbdev);
mp_obj_t common_ColorLight_internal_obj_new(pbio_color_light_t *light);

// pybricks._common.LightArray()
mp_obj_t common_LightArray_obj_make_new(pb_device_t *pbdev, uint8_t light_mode, uint8_t number_of_lights);

#ifdef PYBRICKS_PY_COMMON_LIGHT_MATRIX
#include <pbio/light_matrix.h>
// pybricks._common.LightMatrix()
extern const uint8_t pb_digits_5x2[10][5];
extern const uint8_t pb_font_5x5[95][5];
mp_obj_t pb_type_Lightmatrix_obj_new(pbio_light_matrix_t *light_matrix);
#endif

#if PYBRICKS_PY_COMMON_KEYPAD
// pybricks._common.KeyPad()
mp_obj_t pb_type_Keypad_obj_new(uint8_t number_of_buttons, const pb_obj_enum_member_t **buttons);
#endif

// pybricks._common.Battery()
extern const mp_obj_module_t pb_module_battery;


#if PYBRICKS_PY_COMMON_MOTORS

#include <pbio/control.h>
#include <pbio/servo.h>

// pybricks._common.Control()
extern const mp_obj_type_t pb_type_Control;
mp_obj_t common_Control_obj_make_new(pbio_control_t *control);

// pybricks._common.Logger()
mp_obj_t common_Logger_obj_make_new(pbio_log_t *log, uint8_t num_values);

// pybricks._common.Motor()
typedef struct _common_Motor_obj_t {
    mp_obj_base_t base;
    pbio_servo_t *srv;
    mp_obj_t control;
    mp_obj_t logger;
} common_Motor_obj_t;

extern const mp_obj_type_t pb_type_Motor;

// pybricks._common.DCMotor()
typedef struct _common_DCMotor_obj_t {
    mp_obj_base_t base;
    pbio_dcmotor_t *dcmotor;
} common_DCMotor_obj_t;

extern const mp_obj_type_t pb_type_DCMotor;

// Nonstatic objects shared between Motor and DCMotor
void common_DCMotor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
MP_DECLARE_CONST_FUN_OBJ_KW(common_DCMotor_duty_obj);
MP_DECLARE_CONST_FUN_OBJ_1(common_DCMotor_stop_obj);
MP_DECLARE_CONST_FUN_OBJ_1(common_DCMotor_brake_obj);

#endif // PYBRICKS_PY_COMMON_MOTORS

#if PYBRICKS_PY_COMMON_SPEAKER

extern const mp_obj_type_t pb_type_Speaker;

#endif // PYBRICKS_PY_COMMON_SPEAKER

#if PYBRICKS_PY_COMMON_IMU

mp_obj_t pb_type_IMU_obj_new(mp_obj_t top_side_axis, mp_obj_t front_side_axis);

#endif // PYBRICKS_PY_COMMON_IMU

#endif // PYBRICKS_PY_COMMON

#endif // PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
