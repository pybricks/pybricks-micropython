// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
#define PYBRICKS_INCLUDED_PYBRICKS_COMMON_H

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include "py/obj.h"

#include <pybricks/util_pb/pb_device.h>

// pybricks._common.ColorLight()
const mp_obj_type_t pb_type_ColorLight;
mp_obj_t common_ColorLight_obj_make_new(pb_device_t *pbdev);

// pybricks._common.LightArray()
const mp_obj_type_t pb_type_LightArray;
mp_obj_t common_LightArray_obj_make_new(pb_device_t *pbdev, uint8_t light_mode, uint8_t number_of_lights);

// pybricks._common.KeyPad()
const mp_obj_module_t pb_module_buttons;

// pybricks._common.Battery()
const mp_obj_module_t pb_module_battery;


#if PYBRICKS_PY_COMMON_MOTORS

#include <pbio/control.h>
#include <pbio/servo.h>

// pybricks._common.Control()
const mp_obj_type_t pb_type_Control;
mp_obj_t common_Control_obj_make_new(pbio_control_t *control);

// pybricks._common.Logger()
mp_obj_t logger_obj_make_new(pbio_log_t *log);

// pybricks._common.Motor()
typedef struct _common_Motor_obj_t {
    mp_obj_base_t base;
    pbio_servo_t *srv;
    mp_obj_t logger;
    mp_obj_t control;
} common_Motor_obj_t;

const mp_obj_type_t pb_type_Motor;

// pybricks._common.DCMotor()
typedef struct _common_DCMotor_obj_t {
    mp_obj_base_t base;
    pbio_dcmotor_t *dcmotor;
} common_DCMotor_obj_t;

const mp_obj_type_t pb_type_DCMotor;

// Nonstatic objects shared between Motor and DCMotor
void common_DCMotor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
const mp_obj_fun_builtin_var_t common_DCMotor_duty_obj;
const mp_obj_fun_builtin_fixed_t common_DCMotor_stop_obj;
const mp_obj_fun_builtin_fixed_t common_DCMotor_brake_obj;

#endif // PYBRICKS_PY_COMMON_MOTORS

#endif // PYBRICKS_PY_COMMON

#endif // PYBRICKS_INCLUDED_PYBRICKS_COMMON_H
