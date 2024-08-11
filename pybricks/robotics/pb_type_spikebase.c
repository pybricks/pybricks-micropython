// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE

#include <math.h>
#include <stdlib.h>

#include <pbio/drivebase.h>

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.robotics.SpikeBase class object
typedef struct _robotics_SpikeBase_obj_t {
    mp_obj_base_t base;
    pbio_drivebase_t *db;
    mp_obj_t left;
    mp_obj_t right;
    mp_obj_t heading_control;
    mp_obj_t distance_control;
} robotics_SpikeBase_obj_t;

// pybricks.robotics.SpikeBase.__init__
static mp_obj_t robotics_SpikeBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(left_motor),
        PB_ARG_REQUIRED(right_motor));

    robotics_SpikeBase_obj_t *self = mp_obj_malloc(robotics_SpikeBase_obj_t, type);

    self->left = left_motor_in;
    self->right = right_motor_in;

    // Pointers to servos
    pbio_servo_t *srv_left = pb_type_motor_get_servo(self->left);
    pbio_servo_t *srv_right = pb_type_motor_get_servo(self->right);

    // Create drivebase
    pb_assert(pbio_drivebase_get_drivebase_spike(&self->db, srv_left, srv_right));

    // Create instances of the Control class
    self->heading_control = pb_type_Control_obj_make_new(&self->db->control_heading);
    self->distance_control = pb_type_Control_obj_make_new(&self->db->control_distance);

    return MP_OBJ_FROM_PTR(self);
}

static void wait_for_completion_drivebase(pbio_drivebase_t *db) {
    while (!pbio_drivebase_is_done(db)) {
        mp_hal_delay_ms(5);
    }
    if (!pbio_drivebase_update_loop_is_running(db)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }
}

// pybricks.robotics.SpikeBase.tank_move_for_degrees
static mp_obj_t robotics_SpikeBase_tank_move_for_degrees(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_SpikeBase_obj_t, self,
        PB_ARG_REQUIRED(speed_left),
        PB_ARG_REQUIRED(speed_right),
        PB_ARG_REQUIRED(angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t angle = pb_obj_get_int(angle_in);
    mp_int_t speed_left = pb_obj_get_int(speed_left_in);
    mp_int_t speed_right = pb_obj_get_int(speed_right_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_spike_drive_angle(self->db, speed_left, speed_right, angle, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_tank_move_for_degrees_obj, 1, robotics_SpikeBase_tank_move_for_degrees);

// pybricks.robotics.SpikeBase.steering_move_for_degrees
static mp_obj_t robotics_SpikeBase_steering_move_for_degrees(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_SpikeBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(steering),
        PB_ARG_REQUIRED(angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t angle = pb_obj_get_int(angle_in);
    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t steering = pb_obj_get_int(steering_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Convert steering to tank drive
    int32_t speed_left;
    int32_t speed_right;
    pb_assert(pbio_drivebase_spike_steering_to_tank(speed, steering, &speed_left, &speed_right));

    pb_assert(pbio_drivebase_spike_drive_angle(self->db, speed_left, speed_right, angle, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_steering_move_for_degrees_obj, 1, robotics_SpikeBase_steering_move_for_degrees);

// pybricks.robotics.SpikeBase.tank_move_for_time
static mp_obj_t robotics_SpikeBase_tank_move_for_time(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_SpikeBase_obj_t, self,
        PB_ARG_REQUIRED(speed_left),
        PB_ARG_REQUIRED(speed_right),
        PB_ARG_REQUIRED(time),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t time = pb_obj_get_int(time_in);
    mp_int_t speed_left = pb_obj_get_int(speed_left_in);
    mp_int_t speed_right = pb_obj_get_int(speed_right_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    pb_assert(pbio_drivebase_spike_drive_time(self->db, speed_left, speed_right, time, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_tank_move_for_time_obj, 1, robotics_SpikeBase_tank_move_for_time);

// pybricks.robotics.SpikeBase.steering_move_for_time
static mp_obj_t robotics_SpikeBase_steering_move_for_time(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_SpikeBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(steering),
        PB_ARG_REQUIRED(time),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t time = pb_obj_get_int(time_in);
    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t steering = pb_obj_get_int(steering_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Convert steering to tank drive
    int32_t speed_left;
    int32_t speed_right;
    pb_assert(pbio_drivebase_spike_steering_to_tank(speed, steering, &speed_left, &speed_right));

    pb_assert(pbio_drivebase_spike_drive_time(self->db, speed_left, speed_right, time, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_steering_move_for_time_obj, 1, robotics_SpikeBase_steering_move_for_time);

// pybricks.robotics.SpikeBase.tank_move_forever
static mp_obj_t robotics_SpikeBase_tank_move_forever(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_SpikeBase_obj_t, self,
        PB_ARG_REQUIRED(speed_left),
        PB_ARG_REQUIRED(speed_right));

    // Get wheel diameter and axle track dimensions
    mp_int_t speed_left = pb_obj_get_int(speed_left_in);
    mp_int_t speed_right = pb_obj_get_int(speed_right_in);

    pb_assert(pbio_drivebase_spike_drive_forever(self->db, speed_left, speed_right));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_tank_move_forever_obj, 1, robotics_SpikeBase_tank_move_forever);

// pybricks.robotics.SpikeBase.steering_move_forever
static mp_obj_t robotics_SpikeBase_steering_move_forever(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_SpikeBase_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(steering));

    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t steering = pb_obj_get_int(steering_in);

    // Convert steering to tank drive
    int32_t speed_left;
    int32_t speed_right;
    pb_assert(pbio_drivebase_spike_steering_to_tank(speed, steering, &speed_left, &speed_right));

    pb_assert(pbio_drivebase_spike_drive_forever(self->db, speed_left, speed_right));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_steering_move_forever_obj, 1, robotics_SpikeBase_steering_move_forever);

// pybricks.robotics.SpikeBase.stop
static mp_obj_t robotics_SpikeBase_stop(mp_obj_t self_in) {
    robotics_SpikeBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_drivebase_stop(self->db, PBIO_CONTROL_ON_COMPLETION_COAST));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_SpikeBase_stop_obj, robotics_SpikeBase_stop);

static const pb_attr_dict_entry_t robotics_SpikeBase_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_left, robotics_SpikeBase_obj_t, left),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_right, robotics_SpikeBase_obj_t, right),
    #if PYBRICKS_PY_COMMON_CONTROL
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_heading_control, robotics_SpikeBase_obj_t, heading_control),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_distance_control, robotics_SpikeBase_obj_t, distance_control),
    #endif
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.robotics.SpikeBase)
static const mp_rom_map_elem_t robotics_SpikeBase_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_tank_move_for_degrees),     MP_ROM_PTR(&robotics_SpikeBase_tank_move_for_degrees_obj)     },
    { MP_ROM_QSTR(MP_QSTR_tank_move_for_time),        MP_ROM_PTR(&robotics_SpikeBase_tank_move_for_time_obj)        },
    { MP_ROM_QSTR(MP_QSTR_tank_move_forever),         MP_ROM_PTR(&robotics_SpikeBase_tank_move_forever_obj)             },
    { MP_ROM_QSTR(MP_QSTR_steering_move_for_degrees), MP_ROM_PTR(&robotics_SpikeBase_steering_move_for_degrees_obj) },
    { MP_ROM_QSTR(MP_QSTR_steering_move_for_time),    MP_ROM_PTR(&robotics_SpikeBase_steering_move_for_time_obj)    },
    { MP_ROM_QSTR(MP_QSTR_steering_move_forever),     MP_ROM_PTR(&robotics_SpikeBase_steering_move_forever_obj)     },
    { MP_ROM_QSTR(MP_QSTR_stop),                      MP_ROM_PTR(&robotics_SpikeBase_stop_obj)                      },
};
static MP_DEFINE_CONST_DICT(robotics_SpikeBase_locals_dict, robotics_SpikeBase_locals_dict_table);

// type(pybricks.robotics.SpikeBase)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_spikebase,
    MP_QSTR_SpikeBase,
    MP_TYPE_FLAG_NONE,
    make_new, robotics_SpikeBase_make_new,
    attr, pb_attribute_handler,
    protocol, robotics_SpikeBase_attr_dict,
    locals_dict, &robotics_SpikeBase_locals_dict);

#endif // PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_ROBOTICS_DRIVEBASE_SPIKE
