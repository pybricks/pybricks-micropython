// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS

#include <math.h>
#include <stdlib.h>

#include <pbio/drivebase.h>
#include <pbio/int_math.h>

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
    int32_t initial_distance;
    int32_t initial_heading;
    #if PYBRICKS_PY_COMMON_CONTROL
    mp_obj_t heading_control;
    mp_obj_t distance_control;
    #endif
} robotics_DriveBase_obj_t;

// pybricks.robotics.DriveBase.reset
STATIC mp_obj_t robotics_DriveBase_reset(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, angle, turn_rate;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &drive_speed, &angle, &turn_rate));

    self->initial_distance = distance;
    self->initial_heading = angle;

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_reset_obj, robotics_DriveBase_reset);

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
    pbio_servo_t *srv_left = ((common_Motor_obj_t *)pb_obj_get_base_class_obj(self->left, &pb_type_Motor.type))->srv;
    pbio_servo_t *srv_right = ((common_Motor_obj_t *)pb_obj_get_base_class_obj(self->right, &pb_type_Motor.type))->srv;

    // Create drivebase
    pb_assert(pbio_drivebase_get_drivebase(&self->db, srv_left, srv_right, pb_obj_get_int(wheel_diameter_in), pb_obj_get_int(axle_track_in)));

    #if PYBRICKS_PY_COMMON_CONTROL
    // Create instances of the Control class
    self->heading_control = common_Control_obj_make_new(&self->db->control_heading);
    self->distance_control = common_Control_obj_make_new(&self->db->control_distance);
    #endif

    // Reset drivebase state
    robotics_DriveBase_reset(MP_OBJ_FROM_PTR(self));

    return MP_OBJ_FROM_PTR(self);
}

STATIC void wait_for_completion_drivebase(pbio_drivebase_t *db) {
    while (!pbio_drivebase_is_done(db)) {
        mp_hal_delay_ms(5);
    }
    if (!pbio_drivebase_update_loop_is_running(db)) {
        pb_assert(PBIO_ERROR_NO_DEV);
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
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_drive_straight(self->db, distance, then));

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
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Turning in place is done as a curve with zero radius and a given angle.
    pb_assert(pbio_drivebase_drive_curve(self->db, 0, angle, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_turn_obj, 1, robotics_DriveBase_turn);

// pybricks.robotics.DriveBase.curve
STATIC mp_obj_t robotics_DriveBase_curve(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(radius),
        PB_ARG_REQUIRED(angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t radius = pb_obj_get_int(radius_in);
    mp_int_t angle = pb_obj_get_int(angle_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_drive_curve(self->db, radius, angle, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_curve_obj, 1, robotics_DriveBase_curve);

// pybricks.robotics.DriveBase.drive
STATIC mp_obj_t robotics_DriveBase_drive(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(turn_rate));

    // Get wheel diameter and axle track dimensions
    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t turn_rate = pb_obj_get_int(turn_rate_in);

    pb_assert(pbio_drivebase_drive_forever(self->db, speed, turn_rate));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_drive_obj, 1, robotics_DriveBase_drive);

// pybricks.robotics.DriveBase.stop
STATIC mp_obj_t robotics_DriveBase_stop(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_drivebase_stop(self->db, PBIO_CONTROL_ON_COMPLETION_COAST));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_stop_obj, robotics_DriveBase_stop);

// pybricks.robotics.DriveBase.distance
STATIC mp_obj_t robotics_DriveBase_distance(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, _;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &_, &_, &_));

    return mp_obj_new_int(distance - self->initial_distance);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_distance_obj, robotics_DriveBase_distance);

// pybricks.robotics.DriveBase.angle
STATIC mp_obj_t robotics_DriveBase_angle(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t heading, _;
    pb_assert(pbio_drivebase_get_state_user(self->db, &_, &_, &heading, &_));

    return mp_obj_new_int(heading - self->initial_heading);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_angle_obj, robotics_DriveBase_angle);

// pybricks.robotics.DriveBase.state
STATIC mp_obj_t robotics_DriveBase_state(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, heading, turn_rate;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &drive_speed, &heading, &turn_rate));

    mp_obj_t ret[4];
    ret[0] = mp_obj_new_int(distance - self->initial_distance);
    ret[1] = mp_obj_new_int(drive_speed);
    ret[2] = mp_obj_new_int(heading - self->initial_heading);
    ret[3] = mp_obj_new_int(turn_rate);

    return mp_obj_new_tuple(4, ret);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_state_obj, robotics_DriveBase_state);

// pybricks.robotics.DriveBase.done
STATIC mp_obj_t robotics_DriveBase_done(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(pbio_drivebase_is_done(self->db));
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_done_obj, robotics_DriveBase_done);

// pybricks.robotics.DriveBase.stalled
STATIC mp_obj_t robotics_DriveBase_stalled(mp_obj_t self_in) {
    robotics_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bool stalled;
    uint32_t stall_duration;
    pb_assert(pbio_drivebase_is_stalled(self->db, &stalled, &stall_duration));
    return mp_obj_new_bool(stalled);
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_DriveBase_stalled_obj, robotics_DriveBase_stalled);

// pybricks.robotics.DriveBase.settings
STATIC mp_obj_t robotics_DriveBase_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_DriveBase_obj_t, self,
        PB_ARG_DEFAULT_NONE(straight_speed),
        PB_ARG_DEFAULT_NONE(straight_acceleration),
        PB_ARG_DEFAULT_NONE(turn_rate),
        PB_ARG_DEFAULT_NONE(turn_acceleration));

    // Read acceleration and speed limit settings from control
    int32_t straight_speed, turn_rate;
    int32_t straight_acceleration, turn_acceleration, _;

    // Get current settings. Deceleration values are currently ignored, so not accessible from Python API.
    pbio_drivebase_get_drive_settings(self->db, &straight_speed, &straight_acceleration, &_, &turn_rate, &turn_acceleration, &_);

    // If all given values are none, return current values
    if (straight_speed_in == mp_const_none &&
        straight_acceleration_in == mp_const_none &&
        turn_rate_in == mp_const_none &&
        turn_acceleration_in == mp_const_none
        ) {

        mp_obj_t ret[4];
        ret[0] = mp_obj_new_int(straight_speed);
        ret[1] = mp_obj_new_int(straight_acceleration);
        ret[2] = mp_obj_new_int(turn_rate);
        ret[3] = mp_obj_new_int(turn_acceleration);
        return mp_obj_new_tuple(4, ret);
    }

    // Get the speeds and accelerations if given, bounded by the limit.
    straight_speed = pb_obj_get_default_abs_int(straight_speed_in, straight_speed);
    turn_rate = pb_obj_get_default_abs_int(turn_rate_in, turn_rate);
    straight_acceleration = pb_obj_get_default_abs_int(straight_acceleration_in, straight_acceleration);
    turn_acceleration = pb_obj_get_default_abs_int(turn_acceleration_in, turn_acceleration);

    // Update the settings. Acceleration and deceleration are set to the same acceleration magnitude.
    pbio_drivebase_set_drive_settings(self->db, straight_speed, straight_acceleration, straight_acceleration, turn_rate, turn_acceleration, turn_acceleration);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_DriveBase_settings_obj, 1, robotics_DriveBase_settings);

// dir(pybricks.robotics.DriveBase)
STATIC const mp_rom_map_elem_t robotics_DriveBase_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_curve),            MP_ROM_PTR(&robotics_DriveBase_curve_obj)    },
    { MP_ROM_QSTR(MP_QSTR_straight),         MP_ROM_PTR(&robotics_DriveBase_straight_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn),             MP_ROM_PTR(&robotics_DriveBase_turn_obj)     },
    { MP_ROM_QSTR(MP_QSTR_drive),            MP_ROM_PTR(&robotics_DriveBase_drive_obj)    },
    { MP_ROM_QSTR(MP_QSTR_stop),             MP_ROM_PTR(&robotics_DriveBase_stop_obj)     },
    { MP_ROM_QSTR(MP_QSTR_distance),         MP_ROM_PTR(&robotics_DriveBase_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle),            MP_ROM_PTR(&robotics_DriveBase_angle_obj)    },
    { MP_ROM_QSTR(MP_QSTR_done),             MP_ROM_PTR(&robotics_DriveBase_done_obj)     },
    { MP_ROM_QSTR(MP_QSTR_state),            MP_ROM_PTR(&robotics_DriveBase_state_obj)    },
    { MP_ROM_QSTR(MP_QSTR_reset),            MP_ROM_PTR(&robotics_DriveBase_reset_obj)    },
    { MP_ROM_QSTR(MP_QSTR_settings),         MP_ROM_PTR(&robotics_DriveBase_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_stalled),          MP_ROM_PTR(&robotics_DriveBase_stalled_obj)  },
};
STATIC MP_DEFINE_CONST_DICT(robotics_DriveBase_locals_dict, robotics_DriveBase_locals_dict_table);

STATIC const pb_attr_dict_entry_t robotics_DriveBase_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_left, robotics_DriveBase_obj_t, left),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_right, robotics_DriveBase_obj_t, right),
    #if PYBRICKS_PY_COMMON_CONTROL
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_heading_control, robotics_DriveBase_obj_t, heading_control),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_distance_control, robotics_DriveBase_obj_t, distance_control),
    #endif
};

// type(pybricks.robotics.DriveBase)
const pb_obj_with_attr_type_t pb_type_drivebase = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = MP_QSTR_DriveBase,
        .make_new = robotics_DriveBase_make_new,
        .attr = pb_attribute_handler,
        .locals_dict = (mp_obj_dict_t *)&robotics_DriveBase_locals_dict,
    },
    .attr_dict = robotics_DriveBase_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(robotics_DriveBase_attr_dict),
};

#endif // PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS
