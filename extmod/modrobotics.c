// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <math.h>

#include <pbio/servo.h>
#include <pbio/drivebase.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"
#include "modmotor.h"
#include "pbthread.h"


// Class structure for DriveBase
typedef struct _robotics_DriveBase_obj_t {
    mp_obj_base_t base;
    pbio_drivebase_t *drivebase;
    motor_Motor_obj_t *left;
    motor_Motor_obj_t *right;
    mp_int_t wheel_diameter;
    mp_int_t axle_track;
} robotics_DriveBase_obj_t;

STATIC mp_obj_t robotics_DriveBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(left_motor),
        PB_ARG_REQUIRED(right_motor),
        PB_ARG_REQUIRED(wheel_diameter),
        PB_ARG_REQUIRED(axle_track)
    );

    robotics_DriveBase_obj_t *self = m_new_obj(robotics_DriveBase_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    // Argument must be two motors and two dimensions
    if (!MP_OBJ_IS_TYPE(left_motor, &motor_Motor_type) || !MP_OBJ_IS_TYPE(right_motor, &motor_Motor_type)) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    
    self->left = MP_OBJ_TO_PTR(left_motor);
    self->right = MP_OBJ_TO_PTR(right_motor);

    // Assert that motors can be paired
    pb_assert(pbio_drivebase_get(&self->drivebase, self->left->srv, self->right->srv));

    // Get wheel diameter and axle track dimensions
    self->wheel_diameter = pb_obj_get_int(wheel_diameter);
    self->axle_track = pb_obj_get_int(axle_track);

    // Assert that the dimensions are positive // MOVE TO drivebase as well
    if (self->wheel_diameter < 1 || self->axle_track < 1) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC void robotics_DriveBase_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_DriveBase));
    mp_printf(print, " with left motor on Port %c and right motor on Port %c",
        self->drivebase->left->port, self->drivebase->right->port);
}

STATIC mp_obj_t robotics_DriveBase_drive(mp_obj_t self_in, mp_obj_t speed, mp_obj_t steering) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t sum = pb_obj_get_int(speed)*229/self->wheel_diameter; //TODO: use libfixmath
    mp_int_t dif = 2*self->axle_track*pb_obj_get_int(steering)/self->wheel_diameter;

    pb_thread_enter();
    
    pbio_error_t err_left = pbio_servo_run(self->drivebase->left, (sum+dif)/2);
    pbio_error_t err_right = pbio_servo_run(self->drivebase->right, (sum-dif)/2);

    pb_thread_exit();

    pb_assert(err_left);
    pb_assert(err_right);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(robotics_DriveBase_drive_obj, robotics_DriveBase_drive);

STATIC mp_obj_t robotics_DriveBase_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_INT(stop_type, PBIO_MOTOR_STOP_COAST)
    );
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    pbio_control_after_stop_t after_stop = mp_obj_get_int(stop_type);

    pbio_error_t err_left, err_right;

    pb_thread_enter();

    err_left = pbio_servo_stop(self->drivebase->left, after_stop);
    err_right = pbio_servo_stop(self->drivebase->right, after_stop);

    pb_thread_exit();

    pb_assert(err_left);
    pb_assert(err_right);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_stop_obj, 0, robotics_DriveBase_stop);
/*
DriveBase class tables
*/
STATIC const mp_rom_map_elem_t robotics_DriveBase_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_drive), MP_ROM_PTR(&robotics_DriveBase_drive_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&robotics_DriveBase_stop_obj) },
};
STATIC MP_DEFINE_CONST_DICT(robotics_DriveBase_locals_dict, robotics_DriveBase_locals_dict_table);

STATIC const mp_obj_type_t robotics_DriveBase_type = {
    { &mp_type_type },
    .name = MP_QSTR_DriveBase,
    .print = robotics_DriveBase_print,
    .make_new = robotics_DriveBase_make_new,
    .locals_dict = (mp_obj_dict_t*)&robotics_DriveBase_locals_dict,
};

/*
robotics module tables
*/

STATIC const mp_rom_map_elem_t robotics_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_robotics)         },
    { MP_ROM_QSTR(MP_QSTR_DriveBase),   MP_ROM_PTR(&robotics_DriveBase_type)  },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_robotics_globals, robotics_globals_table);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_robotics_globals,
};

