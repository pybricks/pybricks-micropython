// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS

#include <math.h>
#include <stdlib.h>

#include <pbio/drivebase.h>
#include <pbio/motor_process.h>

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.robotics.DriveBase class object
typedef struct _robotics_DriveBase_obj_t {
    mp_obj_base_t base;
    pbio_drivebase_t *db;
    mp_obj_t left;
    mp_obj_t right;
    mp_obj_t heading_control;
    mp_obj_t distance_control;
    int32_t straight_speed;
    int32_t straight_acceleration;
    int32_t turn_rate;
    int32_t turn_acceleration;
} robotics_DriveBase_obj_t;

// pybricks.robotics.DriveBase.__init__
STATIC mp_obj_t robotics_DriveBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(left_motor),
        PB_ARG_REQUIRED(right_motor),
        PB_ARG_REQUIRED(wheel_diameter),
        PB_ARG_REQUIRED(axle_track));

    robotics_DriveBase_obj_t *self = m_new_obj(robotics_DriveBase_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    self->left = left_motor_in;
    self->right = right_motor_in;

    // Pointers to servos
    pbio_servo_t *srv_left = ((common_Motor_obj_t *)pb_obj_get_base_class_obj(self->left, &pb_type_Motor))->srv;
    pbio_servo_t *srv_right = ((common_Motor_obj_t *)pb_obj_get_base_class_obj(self->right, &pb_type_Motor))->srv;

    // A DriveBase must have two distinct motors
    if (srv_left == srv_right) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Create drivebase
    pb_assert(pbio_motor_process_get_drivebase(&self->db));
    pb_assert(pbio_drivebase_setup(self->db, srv_left, srv_right, pb_obj_get_fix16(wheel_diameter_in), pb_obj_get_fix16(axle_track_in)));

    // Create instances of the Control class
    self->heading_control = common_Control_obj_make_new(&self->db->control_heading);
    self->distance_control = common_Control_obj_make_new(&self->db->control_distance);

    // Get defaults for drivebase as 1/3 of maximum for the underlying motors
    int32_t straight_speed_limit, straight_acceleration_limit, turn_rate_limit, turn_acceleration_limit, _;
    pbio_control_settings_get_limits(&self->db->control_distance.settings, &straight_speed_limit, &straight_acceleration_limit, &_);
    pbio_control_settings_get_limits(&self->db->control_heading.settings, &turn_rate_limit, &turn_acceleration_limit, &_);

    self->straight_speed = straight_speed_limit / 2;
    self->straight_acceleration = straight_acceleration_limit;
    self->turn_rate = turn_rate_limit / 2;
    self->turn_acceleration = turn_acceleration_limit;

    // Reset drivebase state
    pb_assert(pbio_drivebase_reset_state(self->db));

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
        PB_ARG_REQUIRED(distance),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t distance = pb_obj_get_int(distance_in);
    pbio_actuation_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_straight(self->db, distance, self->straight_speed, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_straight_obj, 1, robotics_DriveBase_straight);

// pybricks.robotics.DriveBase.turn
STATIC mp_obj_t robotics_DriveBase_turn(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t angle = pb_obj_get_int(angle_in);
    pbio_actuation_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_turn(self->db, angle, self->turn_rate, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_turn_obj, 1, robotics_DriveBase_turn);

// pybricks.robotics.DriveBase.drive
STATIC mp_obj_t robotics_DriveBase_drive(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(turn_rate));

    // Get wheel diameter and axle track dimensions
    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t turn_rate = pb_obj_get_int(turn_rate_in);

    pb_assert(pbio_drivebase_drive(self->db, speed, turn_rate));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_drive_obj, 1, robotics_DriveBase_drive);

// pybricks._common.DriveBase.stop
STATIC mp_obj_t robotics_DriveBase_stop(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_drivebase_stop(self->db, PBIO_ACTUATION_COAST));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_stop_obj, robotics_DriveBase_stop);

// pybricks._common.DriveBase.distance
STATIC mp_obj_t robotics_DriveBase_distance(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &drive_speed, &angle, &turn_rate));

    return mp_obj_new_int(distance);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_distance_obj, robotics_DriveBase_distance);

// pybricks._common.DriveBase.angle
STATIC mp_obj_t robotics_DriveBase_angle(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &drive_speed, &angle, &turn_rate));

    return mp_obj_new_int(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_angle_obj, robotics_DriveBase_angle);

// pybricks._common.DriveBase.state
STATIC mp_obj_t robotics_DriveBase_state(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &drive_speed, &angle, &turn_rate));

    mp_obj_t ret[4];
    ret[0] = mp_obj_new_int(distance);
    ret[1] = mp_obj_new_int(drive_speed);
    ret[2] = mp_obj_new_int(angle);
    ret[3] = mp_obj_new_int(turn_rate);

    return mp_obj_new_tuple(4, ret);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_state_obj, robotics_DriveBase_state);


// pybricks._common.DriveBase.reset
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
        PB_ARG_DEFAULT_NONE(turn_acceleration));

    // If all given values are none, return current values
    if (straight_speed_in == mp_const_none &&
        straight_acceleration_in == mp_const_none &&
        turn_rate_in == mp_const_none &&
        turn_acceleration_in == mp_const_none
        ) {
        mp_obj_t ret[4];
        ret[0] = mp_obj_new_int(self->straight_speed);
        ret[1] = mp_obj_new_int(self->straight_acceleration);
        ret[2] = mp_obj_new_int(self->turn_rate);
        ret[3] = mp_obj_new_int(self->turn_acceleration);
        return mp_obj_new_tuple(4, ret);
    }

    if (self->db->control_distance.type != PBIO_CONTROL_NONE || self->db->control_heading.type != PBIO_CONTROL_NONE) {
        pb_assert(PBIO_ERROR_BUSY);
    }

    // If some values are given, set them, bound by the control limits
    int32_t straight_speed_limit, straight_acceleration_limit, turn_rate_limit, turn_acceleration_limit, _;
    pbio_control_settings_get_limits(&self->db->control_distance.settings, &straight_speed_limit, &straight_acceleration_limit, &_);
    pbio_control_settings_get_limits(&self->db->control_heading.settings, &turn_rate_limit, &turn_acceleration_limit, &_);

    self->straight_speed = min(straight_speed_limit, abs(pb_obj_get_default_int(straight_speed_in, self->straight_speed)));
    self->straight_acceleration = min(straight_acceleration_limit, abs(pb_obj_get_default_int(straight_acceleration_in, self->straight_acceleration)));
    self->turn_rate = min(turn_rate_limit, abs(pb_obj_get_default_int(turn_rate_in, self->turn_rate)));
    self->turn_acceleration = min(turn_acceleration_limit, abs(pb_obj_get_default_int(turn_acceleration_in, self->turn_acceleration)));

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
    { MP_ROM_QSTR(MP_QSTR_heading_control),  MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, heading_control) },
    { MP_ROM_QSTR(MP_QSTR_distance_control), MP_ROM_ATTRIBUTE_OFFSET(robotics_DriveBase_obj_t, distance_control)},
};
STATIC MP_DEFINE_CONST_DICT(robotics_DriveBase_locals_dict, robotics_DriveBase_locals_dict_table);

// type(pybricks.robotics.DriveBase)
const mp_obj_type_t pb_type_drivebase = {
    { &mp_type_type },
    .name = MP_QSTR_DriveBase,
    .make_new = robotics_DriveBase_make_new,
    .locals_dict = (mp_obj_dict_t *)&robotics_DriveBase_locals_dict,
};

#endif // PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS
