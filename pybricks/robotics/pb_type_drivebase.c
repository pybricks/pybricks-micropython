// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS

#include <math.h>
#include <stdlib.h>

#include <pbio/drivebase.h>
#include <pbio/int_math.h>

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/robotics.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_async.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct _pb_type_DriveBase_obj_t pb_type_DriveBase_obj_t;

// pybricks.robotics.DriveBase class object
struct _pb_type_DriveBase_obj_t {
    mp_obj_base_t base;
    pbio_drivebase_t *db;
    #if PYBRICKS_PY_COMMON_CONTROL
    mp_obj_t heading_control;
    mp_obj_t distance_control;
    #endif
    pb_type_async_t *last_awaitable;
};

// pybricks.robotics.DriveBase.reset
static mp_obj_t pb_type_DriveBase_reset(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_DEFAULT_INT(distance, 0),
        PB_ARG_DEFAULT_INT(angle, 0));

    pb_assert(pbio_drivebase_reset(self->db, pb_obj_get_int(distance_in), pb_obj_get_int(angle_in)));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_reset_obj, 1, pb_type_DriveBase_reset);

// pybricks.robotics.DriveBase.__init__
static mp_obj_t pb_type_DriveBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(left_motor),
        PB_ARG_REQUIRED(right_motor),
        PB_ARG_REQUIRED(wheel_diameter),
        PB_ARG_REQUIRED(axle_track));

    pb_type_DriveBase_obj_t *self = mp_obj_malloc(pb_type_DriveBase_obj_t, type);

    // Pointers to servos
    pbio_servo_t *srv_left = pb_type_motor_get_servo(left_motor_in);
    pbio_servo_t *srv_right = pb_type_motor_get_servo(right_motor_in);

    // Create drivebase. Initialized to use motor encoders (not gyro) for heading.
    pb_assert(pbio_drivebase_get_drivebase(&self->db,
        srv_left,
        srv_right,
        pb_obj_get_scaled_int(wheel_diameter_in, 1000),
        pb_obj_get_scaled_int(axle_track_in, 1000)));

    #if PYBRICKS_PY_COMMON_CONTROL
    // Create instances of the Control class
    self->heading_control = pb_type_Control_obj_make_new(&self->db->control_heading);
    self->distance_control = pb_type_Control_obj_make_new(&self->db->control_distance);
    #endif

    self->last_awaitable = NULL;

    return MP_OBJ_FROM_PTR(self);
}

static pbio_error_t pb_type_drivebase_iterate_once(pbio_os_state_t *state, mp_obj_t parent_obj) {
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(parent_obj);

    // Handle I/O exceptions like port unplugged.
    if (!pbio_drivebase_update_loop_is_running(self->db)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Get completion state.
    return pbio_drivebase_is_done(self->db) ? PBIO_SUCCESS : PBIO_ERROR_AGAIN;
}

// pybricks.robotics.DriveBase.stop
static mp_obj_t pb_type_DriveBase_stop(mp_obj_t self_in) {

    // Cancel awaitables.
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_async_schedule_stop_iteration(self->last_awaitable);

    // Stop hardware.
    pb_assert(pbio_drivebase_stop(self->db, PBIO_CONTROL_ON_COMPLETION_COAST));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_DriveBase_stop_obj, pb_type_DriveBase_stop);


// All drive base methods use the same kind of completion awaitable.
static mp_obj_t await_or_wait(pb_type_DriveBase_obj_t *self) {

    pb_type_async_t config = {
        .parent_obj = MP_OBJ_FROM_PTR(self),
        .iter_once = pb_type_drivebase_iterate_once,
        .close = pb_type_DriveBase_stop,
    };
    // New operation always wins; ongoing awaitable motion is cancelled.
    return pb_type_async_wait_or_await(&config, &self->last_awaitable, true);
}

// pybricks.robotics.DriveBase.straight
static mp_obj_t pb_type_DriveBase_straight(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(distance),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t distance = pb_obj_get_int(distance_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_drive_straight(self->db, distance, then));

    // Old way to do parallel movement is to start and not wait on anything.
    if (!mp_obj_is_true(wait_in)) {
        return mp_const_none;
    }
    // Handle completion by awaiting or blocking.
    return await_or_wait(self);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_straight_obj, 1, pb_type_DriveBase_straight);

// pybricks.robotics.DriveBase.turn
static mp_obj_t pb_type_DriveBase_turn(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t angle = pb_obj_get_int(angle_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Turning in place is done as a curve with zero radius and a given angle.
    pb_assert(pbio_drivebase_drive_curve(self->db, 0, angle, then));

    // Old way to do parallel movement is to start and not wait on anything.
    if (!mp_obj_is_true(wait_in)) {
        return mp_const_none;
    }
    // Handle completion by awaiting or blocking.
    return await_or_wait(self);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_turn_obj, 1, pb_type_DriveBase_turn);

// pybricks.robotics.DriveBase.curve
static mp_obj_t pb_type_DriveBase_curve(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(radius),
        PB_ARG_REQUIRED(angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t radius = pb_obj_get_int(radius_in);
    mp_int_t angle = pb_obj_get_int(angle_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_drive_curve(self->db, radius, angle, then));

    // Old way to do parallel movement is to start and not wait on anything.
    if (!mp_obj_is_true(wait_in)) {
        return mp_const_none;
    }
    // Handle completion by awaiting or blocking.
    return await_or_wait(self);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_curve_obj, 1, pb_type_DriveBase_curve);

// pybricks.robotics.DriveBase.arc
static mp_obj_t pb_type_DriveBase_arc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(radius),
        PB_ARG_DEFAULT_NONE(angle),
        PB_ARG_DEFAULT_NONE(distance),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    // Parse user arguments.
    mp_int_t radius = pb_obj_get_int(radius_in);
    if ((distance_in == mp_const_none) == (angle_in == mp_const_none)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Please specify distance or angle but not both."));
    }

    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    if (distance_in != mp_const_none) {
        pb_assert(pbio_drivebase_drive_arc_distance(self->db, radius, pb_obj_get_int(distance_in), then));
    } else {
        pb_assert(pbio_drivebase_drive_arc_angle(self->db, radius, pb_obj_get_int(angle_in), then));
    }

    // Old way to do parallel movement is to start and not wait on anything.
    if (!mp_obj_is_true(wait_in)) {
        return mp_const_none;
    }
    // Handle completion by awaiting or blocking.
    return await_or_wait(self);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_arc_obj, 1, pb_type_DriveBase_arc);

// pybricks.robotics.DriveBase.drive
static mp_obj_t pb_type_DriveBase_drive(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(turn_rate));

    // Get wheel diameter and axle track dimensions
    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t turn_rate = pb_obj_get_int(turn_rate_in);

    // Cancel awaitables but not hardware. Drive forever will handle this.
    pb_type_async_schedule_stop_iteration(self->last_awaitable);

    pb_assert(pbio_drivebase_drive_forever(self->db, speed, turn_rate));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_drive_obj, 1, pb_type_DriveBase_drive);

// pybricks.robotics.DriveBase.brake
static mp_obj_t pb_type_DriveBase_brake(mp_obj_t self_in) {

    // Cancel awaitables.
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_async_schedule_stop_iteration(self->last_awaitable);

    // Stop hardware.
    pb_assert(pbio_drivebase_stop(self->db, PBIO_CONTROL_ON_COMPLETION_BRAKE));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_DriveBase_brake_obj, pb_type_DriveBase_brake);

// pybricks.robotics.DriveBase.distance
static mp_obj_t pb_type_DriveBase_distance(mp_obj_t self_in) {
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, _;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &_, &_, &_));

    return mp_obj_new_int(distance);
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_DriveBase_distance_obj, pb_type_DriveBase_distance);

// pybricks.robotics.DriveBase.angle
static mp_obj_t pb_type_DriveBase_angle(mp_obj_t self_in) {
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    #if MICROPY_PY_BUILTINS_FLOAT
    float angle;
    pb_assert(pbio_drivebase_get_state_user_angle(self->db, &angle));
    return mp_obj_new_float_from_f(angle);
    #else
    int32_t heading, _;
    pb_assert(pbio_drivebase_get_state_user(self->db, &_, &_, &heading, &_));
    return mp_obj_new_int(heading);
    #endif
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_DriveBase_angle_obj, pb_type_DriveBase_angle);

// pybricks.robotics.DriveBase.state
static mp_obj_t pb_type_DriveBase_state(mp_obj_t self_in) {
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t distance, drive_speed, heading, turn_rate;
    pb_assert(pbio_drivebase_get_state_user(self->db, &distance, &drive_speed, &heading, &turn_rate));

    mp_obj_t ret[4];
    ret[0] = mp_obj_new_int(distance);
    ret[1] = mp_obj_new_int(drive_speed);
    ret[2] = mp_obj_new_int(heading);
    ret[3] = mp_obj_new_int(turn_rate);

    return mp_obj_new_tuple(4, ret);
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_DriveBase_state_obj, pb_type_DriveBase_state);

// pybricks.robotics.DriveBase.done
static mp_obj_t pb_type_DriveBase_done(mp_obj_t self_in) {
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(pbio_drivebase_is_done(self->db));
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_DriveBase_done_obj, pb_type_DriveBase_done);

// pybricks.robotics.DriveBase.stalled
static mp_obj_t pb_type_DriveBase_stalled(mp_obj_t self_in) {
    pb_type_DriveBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bool stalled;
    uint32_t stall_duration;
    pb_assert(pbio_drivebase_is_stalled(self->db, &stalled, &stall_duration));
    return mp_obj_new_bool(stalled);
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_DriveBase_stalled_obj, pb_type_DriveBase_stalled);

// pybricks.robotics.DriveBase.settings
static mp_obj_t pb_type_DriveBase_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_DEFAULT_NONE(straight_speed),
        PB_ARG_DEFAULT_NONE(straight_acceleration),
        PB_ARG_DEFAULT_NONE(turn_rate),
        PB_ARG_DEFAULT_NONE(turn_acceleration));

    // Read acceleration and speed limit settings from control
    int32_t straight_speed, turn_rate;
    int32_t straight_acceleration, turn_acceleration;
    int32_t straight_deceleration, turn_deceleration;

    // Get current settings.
    pbio_drivebase_get_drive_settings(self->db,
        &straight_speed, &straight_acceleration, &straight_deceleration,
        &turn_rate, &turn_acceleration, &turn_deceleration);

    // If all given values are none, return current values
    if (PB_PARSE_ARGS_METHOD_ALL_NONE()) {
        mp_obj_t ret[] = {
            mp_obj_new_int(straight_speed),
            make_acceleration_return_value(straight_acceleration, straight_deceleration),
            mp_obj_new_int(turn_rate),
            make_acceleration_return_value(turn_acceleration, turn_deceleration),
        };
        return mp_obj_new_tuple(MP_ARRAY_SIZE(ret), ret);
    }

    // Get the speeds and accelerations if given, bounded by the limit.
    straight_speed = pb_obj_get_default_abs_int(straight_speed_in, straight_speed);
    turn_rate = pb_obj_get_default_abs_int(turn_rate_in, turn_rate);
    unpack_acceleration_value(straight_acceleration_in, &straight_acceleration, &straight_deceleration);
    unpack_acceleration_value(turn_acceleration_in, &turn_acceleration, &turn_deceleration);

    // Update the settings. Acceleration and deceleration are set to the same acceleration magnitude.
    pb_assert(pbio_drivebase_set_drive_settings(self->db,
        straight_speed, straight_acceleration, straight_deceleration,
        turn_rate, turn_acceleration, turn_deceleration));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_settings_obj, 1, pb_type_DriveBase_settings);

#if PYBRICKS_PY_ROBOTICS_DRIVEBASE_GYRO
// pybricks.robotics.DriveBase.use_gyro
static mp_obj_t pb_type_DriveBase_use_gyro(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_DriveBase_obj_t, self,
        PB_ARG_REQUIRED(use_gyro));

    pbio_imu_heading_type_t type = PBIO_IMU_HEADING_TYPE_NONE;
    if (mp_obj_is_true(use_gyro_in)) {
        type = PBIO_IMU_HEADING_TYPE_3D;
    }

    pb_assert(pbio_drivebase_set_use_gyro(self->db, type));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_DriveBase_use_gyro_obj, 1, pb_type_DriveBase_use_gyro);
#endif

#if PYBRICKS_PY_COMMON_CONTROL
static const pb_attr_dict_entry_t pb_type_DriveBase_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_heading_control, pb_type_DriveBase_obj_t, heading_control),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_distance_control, pb_type_DriveBase_obj_t, distance_control),
    PB_ATTR_DICT_SENTINEL
};
#endif

// dir(pybricks.robotics.DriveBase)
static const mp_rom_map_elem_t pb_type_DriveBase_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_arc),              MP_ROM_PTR(&pb_type_DriveBase_arc_obj)      },
    { MP_ROM_QSTR(MP_QSTR_curve),            MP_ROM_PTR(&pb_type_DriveBase_curve_obj)    },
    { MP_ROM_QSTR(MP_QSTR_straight),         MP_ROM_PTR(&pb_type_DriveBase_straight_obj) },
    { MP_ROM_QSTR(MP_QSTR_turn),             MP_ROM_PTR(&pb_type_DriveBase_turn_obj)     },
    { MP_ROM_QSTR(MP_QSTR_drive),            MP_ROM_PTR(&pb_type_DriveBase_drive_obj)    },
    { MP_ROM_QSTR(MP_QSTR_stop),             MP_ROM_PTR(&pb_type_DriveBase_stop_obj)     },
    { MP_ROM_QSTR(MP_QSTR_brake),            MP_ROM_PTR(&pb_type_DriveBase_brake_obj)    },
    { MP_ROM_QSTR(MP_QSTR_distance),         MP_ROM_PTR(&pb_type_DriveBase_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle),            MP_ROM_PTR(&pb_type_DriveBase_angle_obj)    },
    { MP_ROM_QSTR(MP_QSTR_done),             MP_ROM_PTR(&pb_type_DriveBase_done_obj)     },
    { MP_ROM_QSTR(MP_QSTR_state),            MP_ROM_PTR(&pb_type_DriveBase_state_obj)    },
    { MP_ROM_QSTR(MP_QSTR_reset),            MP_ROM_PTR(&pb_type_DriveBase_reset_obj)    },
    { MP_ROM_QSTR(MP_QSTR_settings),         MP_ROM_PTR(&pb_type_DriveBase_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_stalled),          MP_ROM_PTR(&pb_type_DriveBase_stalled_obj)  },
    #if PYBRICKS_PY_ROBOTICS_DRIVEBASE_GYRO
    { MP_ROM_QSTR(MP_QSTR_use_gyro),         MP_ROM_PTR(&pb_type_DriveBase_use_gyro_obj) },
    #endif
};
// First N entries are common to both drive base classes.
static MP_DEFINE_CONST_DICT(pb_type_DriveBase_locals_dict, pb_type_DriveBase_locals_dict_table);

// type(pybricks.robotics.DriveBase)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_drivebase,
    MP_QSTR_DriveBase,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_DriveBase_make_new,
    #if PYBRICKS_PY_COMMON_CONTROL
    attr, pb_attribute_handler,
    protocol, pb_type_DriveBase_attr_dict,
    #endif
    locals_dict, &pb_type_DriveBase_locals_dict);

#endif // PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS
