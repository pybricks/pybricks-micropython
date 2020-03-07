// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk

#include <math.h>

#include <pbio/drivebase.h>
#include <pbio/motorpoll.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"

#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

#include "modparameters.h"
#include "modbuiltins.h"
#include "modmotor.h"
#include "modlogger.h"

#if PYBRICKS_PY_ROBOTICS

// pybricks.robotics.DriveBase class object
typedef struct _robotics_DriveBase_obj_t {
    mp_obj_base_t base;
    pbio_drivebase_t *db;
    motor_Motor_obj_t *left;
    motor_Motor_obj_t *right;
    mp_obj_t logger;
    mp_obj_t heading_control;
    mp_obj_t distance_control;
    int32_t straight_speed;
    int32_t straight_acceleration;
    int32_t turn_rate;
    int32_t turn_acceleration;
} robotics_DriveBase_obj_t;

// pybricks.robotics.DriveBase.__init__
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

    // Pointer to the Python (not pbio) Motor objects
    self->left = MP_OBJ_TO_PTR(left_motor);
    self->right = MP_OBJ_TO_PTR(right_motor);

    // Get wheel diameter and axle track dimensions
    fix16_t wheel_diameter_val = pb_obj_get_fix16(wheel_diameter);
    fix16_t axle_track_val = pb_obj_get_fix16(axle_track);

    // Create drivebase
    pb_assert(pbio_motorpoll_get_drivebase(&self->db));
    pb_assert(pbio_drivebase_setup(self->db, self->left->srv, self->right->srv, wheel_diameter_val, axle_track_val));
    pb_assert(pbio_motorpoll_set_drivebase_status(self->db, PBIO_ERROR_AGAIN));

    // Create an instance of the Logger class
    self->logger = logger_obj_make_new(&self->db->log);

    // Create instances of the Control class
    self->heading_control = builtins_Control_obj_make_new(&self->db->control_heading);
    self->distance_control = builtins_Control_obj_make_new(&self->db->control_distance);

    // Set defaults for drivebase
    self->straight_speed = 100;
    self->straight_acceleration = 200;
    self->turn_rate = 90;
    self->turn_acceleration = 180;

    return MP_OBJ_FROM_PTR(self);
}

STATIC void wait_for_completion_drivebase(pbio_drivebase_t *db) {
    while (!pbio_control_is_done(&db->control_distance) || !pbio_control_is_done(&db->control_heading)) {
        mp_hal_delay_ms(5);
    }
}

// pybricks.robotics.DriveBase.straight
STATIC mp_obj_t robotics_DriveBase_straight(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(distance)
    );

    int32_t distance_val = pb_obj_get_int(distance);
    pb_assert(pbio_drivebase_straight(self->db, distance_val, self->straight_speed, self->straight_acceleration));

    wait_for_completion_drivebase(self->db);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_straight_obj, 1, robotics_DriveBase_straight);

// pybricks.robotics.DriveBase.turn
STATIC mp_obj_t robotics_DriveBase_turn(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(angle)
    );

    int32_t angle_val = pb_obj_get_int(angle);
    pb_assert(pbio_drivebase_turn(self->db, angle_val, self->turn_rate, self->turn_acceleration));

    wait_for_completion_drivebase(self->db);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_turn_obj, 1, robotics_DriveBase_turn);

// pybricks.robotics.DriveBase.drive
STATIC mp_obj_t robotics_DriveBase_drive(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(turn_rate)
    );

    // Get wheel diameter and axle track dimensions
    int32_t speed_val = pb_obj_get_int(speed);
    int32_t turn_rate_val = pb_obj_get_int(turn_rate);

    pb_assert(pbio_drivebase_drive(self->db, speed_val, turn_rate_val));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_drive_obj, 1, robotics_DriveBase_drive);

// pybricks.robotics.DriveBase.stop
STATIC mp_obj_t robotics_DriveBase_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_DEFAULT_OBJ(stop_type, pb_Stop_HOLD_obj)
    );

    pbio_actuation_t after_stop = pb_type_enum_get_value(stop_type, &pb_enum_type_Stop);
    pb_assert(pbio_drivebase_stop(self->db, after_stop));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_stop_obj, 1, robotics_DriveBase_stop);

// pybricks.builtins.DriveBase.distance
STATIC mp_obj_t robotics_DriveBase_distance(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state(self->db, &distance, &drive_speed, &angle, &turn_rate));

    return mp_obj_new_int(distance);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_distance_obj, robotics_DriveBase_distance);

// pybricks.builtins.DriveBase.angle
STATIC mp_obj_t robotics_DriveBase_angle(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state(self->db, &distance, &drive_speed, &angle, &turn_rate));

    return mp_obj_new_int(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_angle_obj, robotics_DriveBase_angle);

// pybricks.builtins.DriveBase.state
STATIC mp_obj_t robotics_DriveBase_state(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state(self->db, &distance, &drive_speed, &angle, &turn_rate));

    mp_obj_t ret[4];
    ret[0] = mp_obj_new_int(distance);
    ret[1] = mp_obj_new_int(drive_speed);
    ret[2] = mp_obj_new_int(angle);
    ret[3] = mp_obj_new_int(turn_rate);

    return mp_obj_new_tuple(4, ret);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_state_obj, robotics_DriveBase_state);


// pybricks.builtins.DriveBase.reset
STATIC mp_obj_t robotics_DriveBase_reset(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_assert(pbio_drivebase_reset_state(self->db));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_reset_obj, robotics_DriveBase_reset);

// pybricks.robotics.DriveBase.settings
STATIC mp_obj_t robotics_DriveBase_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_DEFAULT_NONE(straight_speed),
        PB_ARG_DEFAULT_NONE(straight_acceleration),
        PB_ARG_DEFAULT_NONE(turn_rate),
        PB_ARG_DEFAULT_NONE(turn_acceleration)
    );

    // If all given values are none, return current values
    if (straight_speed == mp_const_none &&
        straight_acceleration == mp_const_none &&
        turn_rate == mp_const_none &&
        turn_acceleration == mp_const_none
    ) {
        mp_obj_t ret[4];
        ret[0] = mp_obj_new_int(self->straight_speed);
        ret[1] = mp_obj_new_int(self->straight_acceleration);
        ret[2] = mp_obj_new_int(self->turn_rate);
        ret[3] = mp_obj_new_int(self->turn_acceleration);
        return mp_obj_new_tuple(4, ret);
    }

    if (self->db->control_distance.type != PBIO_CONTROL_NONE || self->db->control_heading.type != PBIO_CONTROL_NONE) {
        pb_assert(PBIO_ERROR_INVALID_OP);
    }

    // If some values are given, set them
    self->straight_speed = pb_obj_get_default_int(straight_speed, self->straight_speed);
    self->straight_acceleration = pb_obj_get_default_int(straight_acceleration, self->straight_acceleration);
    self->turn_rate = pb_obj_get_default_int(turn_rate, self->turn_rate);
    self->turn_acceleration = pb_obj_get_default_int(turn_rate, self->turn_acceleration);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_settings_obj, 1, robotics_DriveBase_settings);

// dir(pybricks.robotics.DriveBase)
STATIC const mp_rom_map_elem_t robotics_DriveBase_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_straight),         MP_ROM_PTR(&robotics_DriveBase_straight_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn),             MP_ROM_PTR(&robotics_DriveBase_turn_obj)     },
    { MP_ROM_QSTR(MP_QSTR_drive),            MP_ROM_PTR(&robotics_DriveBase_drive_obj)    },
    { MP_ROM_QSTR(MP_QSTR_stop),             MP_ROM_PTR(&robotics_DriveBase_stop_obj)     },
    { MP_ROM_QSTR(MP_QSTR_distance),         MP_ROM_PTR(&robotics_DriveBase_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle),            MP_ROM_PTR(&robotics_DriveBase_angle_obj)    },
    { MP_ROM_QSTR(MP_QSTR_state),            MP_ROM_PTR(&robotics_DriveBase_state_obj)    },
    { MP_ROM_QSTR(MP_QSTR_reset),            MP_ROM_PTR(&robotics_DriveBase_reset_obj)    },
    { MP_ROM_QSTR(MP_QSTR_settings),         MP_ROM_PTR(&robotics_DriveBase_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_left),             MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, left)            },
    { MP_ROM_QSTR(MP_QSTR_right),            MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, right)           },
    { MP_ROM_QSTR(MP_QSTR_log),              MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, logger)          },
    { MP_ROM_QSTR(MP_QSTR_heading_control),  MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, heading_control) },
    { MP_ROM_QSTR(MP_QSTR_distance_control), MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, distance_control)},
};
STATIC MP_DEFINE_CONST_DICT(robotics_DriveBase_locals_dict, robotics_DriveBase_locals_dict_table);

// type(pybricks.robotics.DriveBase)
STATIC const mp_obj_type_t robotics_DriveBase_type = {
    { &mp_type_type },
    .name = MP_QSTR_DriveBase,
    .make_new = robotics_DriveBase_make_new,
    .locals_dict = (mp_obj_dict_t*)&robotics_DriveBase_locals_dict,
};

// dir(pybricks.robotics)
STATIC const mp_rom_map_elem_t robotics_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_robotics)         },
    { MP_ROM_QSTR(MP_QSTR_DriveBase),   MP_ROM_PTR(&robotics_DriveBase_type)  },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_robotics_globals, robotics_globals_table);

const mp_obj_module_t pb_module_robotics = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_robotics_globals,
};

#endif // PYBRICKS_PY_ROBOTICS
