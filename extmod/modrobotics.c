// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <math.h>

#include <pbio/servo.h>
#include <pbio/drivebase.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "pberror.h"
#include "pbobj.h"
#include "modmotor.h"
#include "pbthread.h"


// Class structure for DriveBase
typedef struct _robotics_DriveBase_obj_t {
    mp_obj_base_t base;
    motor_Motor_obj_t *mtr_left;
    motor_Motor_obj_t *mtr_right;
    mp_int_t wheel_diameter;
    mp_int_t axle_track;
} robotics_DriveBase_obj_t;

STATIC mp_obj_t robotics_DriveBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    robotics_DriveBase_obj_t *self = m_new_obj(robotics_DriveBase_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    // We should have four arguments
    mp_arg_check_num(n_args, n_kw, 4, 4, false);

    // Argument must be two motors and two dimensions
    if (!MP_OBJ_IS_TYPE(args[0], &motor_Motor_type) || !MP_OBJ_IS_TYPE(args[1], &motor_Motor_type)) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    self->mtr_left = MP_OBJ_TO_PTR(args[0]);
    self->mtr_right = MP_OBJ_TO_PTR(args[1]);

    // Assert that motors can be paired
    pbio_motor_pair_t pair;
    pb_assert(pbio_get_motor_pair(self->mtr_left->mtr, self->mtr_right->mtr, &pair));

    // Get wheel diameter and axle track dimensions
    self->wheel_diameter = pb_obj_get_int(args[2]);
    self->axle_track = pb_obj_get_int(args[3]);

    // Assert that the dimensions are positive
    if (self->wheel_diameter < 1 || self->axle_track < 1) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC void robotics_DriveBase_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_DriveBase));
    mp_printf(print, " with left motor on Port %c and right motor on Port %c",
        self->mtr_left->mtr->port, self->mtr_right->mtr->port);
}

STATIC mp_obj_t robotics_DriveBase_drive(mp_obj_t self_in, mp_obj_t speed, mp_obj_t steering) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t sum = pb_obj_get_int(speed)*229/self->wheel_diameter; //TODO: use libfixmath
    mp_int_t dif = 2*self->axle_track*pb_obj_get_int(steering)/self->wheel_diameter;

    pb_thread_enter();

    pbio_error_t err_left = pbio_motor_run(self->mtr_left->mtr, (sum+dif)/2);
    pbio_error_t err_right = pbio_motor_run(self->mtr_right->mtr, (sum-dif)/2);

    pb_thread_exit();

    pb_assert(err_left);
    pb_assert(err_right);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(robotics_DriveBase_drive_obj, robotics_DriveBase_drive);

STATIC mp_obj_t robotics_DriveBase_stop(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    pbio_control_after_stop_t after_stop = n_args > 1 ? mp_obj_get_int(args[1]) : PBIO_MOTOR_STOP_COAST;
    pbio_error_t err_left, err_right;

    pb_thread_enter();

    err_left = pbio_motor_stop(self->mtr_left->mtr, after_stop);
    err_right = pbio_motor_stop(self->mtr_right->mtr, after_stop);

    pb_thread_exit();

    pb_assert(err_left);
    pb_assert(err_right);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(robotics_DriveBase_stop_obj, 1, 2, robotics_DriveBase_stop);

STATIC mp_obj_t robotics_DriveBase_drive_time(size_t n_args, const mp_obj_t *args){
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    robotics_DriveBase_drive(self, args[1], args[2]);
    mp_hal_delay_ms(pb_obj_get_int(args[3]));
    // TODO: parse stop type
    robotics_DriveBase_drive(self, MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_NEW_SMALL_INT(0));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(robotics_DriveBase_drive_time_obj, 4, 5, robotics_DriveBase_drive_time);

/*
DriveBase class tables
*/
STATIC const mp_rom_map_elem_t robotics_DriveBase_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_drive), MP_ROM_PTR(&robotics_DriveBase_drive_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&robotics_DriveBase_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_drive_time), MP_ROM_PTR(&robotics_DriveBase_drive_time_obj) },
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

