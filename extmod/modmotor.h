// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbio/servo.h>

#include "py/obj.h"

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

// pybricks.builtins.Motor class object
typedef struct _motor_Motor_obj_t {
    mp_obj_base_t base;
    pbio_servo_t *srv;
    mp_obj_t logger;
    mp_obj_t control;
} motor_Motor_obj_t;

const mp_obj_type_t motor_Motor_type;

// pybricks.builtins.DCMotor class object
typedef struct _motor_DCMotor_obj_t {
    mp_obj_base_t base;
    pbio_dcmotor_t *dcmotor;
} motor_DCMotor_obj_t;

const mp_obj_type_t motor_DCMotor_type;

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
