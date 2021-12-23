// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS && (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)

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
STATIC mp_obj_t robotics_SpikeBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(left_motor),
        PB_ARG_REQUIRED(right_motor));

    robotics_SpikeBase_obj_t *self = m_new_obj(robotics_SpikeBase_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    self->left = left_motor_in;
    self->right = right_motor_in;

    // Pointers to servos
    pbio_servo_t *srv_left = ((common_Motor_obj_t *)pb_obj_get_base_class_obj(self->left, &pb_type_Motor))->srv;
    pbio_servo_t *srv_right = ((common_Motor_obj_t *)pb_obj_get_base_class_obj(self->right, &pb_type_Motor))->srv;

    // Create drivebase
    pb_assert(pbio_drivebase_get_spikebase(&self->db, srv_left, srv_right));

    // Create instances of the Control class
    self->heading_control = common_Control_obj_make_new(&self->db->control_heading);
    self->distance_control = common_Control_obj_make_new(&self->db->control_distance);

    return MP_OBJ_FROM_PTR(self);
}

STATIC void wait_for_completion_drivebase(pbio_drivebase_t *db) {
    while (pbio_drivebase_is_busy(db)) {
        mp_hal_delay_ms(5);
    }
    if (!pbio_drivebase_update_loop_is_running(db)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }
}

// pybricks.robotics.SpikeBase.tank_move_for_degrees
STATIC mp_obj_t robotics_SpikeBase_tank_move_for_degrees(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
    pbio_actuation_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Driving tank_move_for_degrees is done as a curve with infinite radius and a given distance.
    pb_assert(pbio_spikebase_drive_angle(self->db, speed_left, speed_right, angle, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_tank_move_for_degrees_obj, 1, robotics_SpikeBase_tank_move_for_degrees);

// pybricks.robotics.SpikeBase.tank_move_for_time
STATIC mp_obj_t robotics_SpikeBase_tank_move_for_time(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
    pbio_actuation_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Driving tank_move_for_time is done as a curve with infinite radius and a given distance.
    pb_assert(pbio_spikebase_drive_time(self->db, speed_left, speed_right, time, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_tank_move_for_time_obj, 1, robotics_SpikeBase_tank_move_for_time);

// pybricks.robotics.SpikeBase.steering_move_for_degrees
STATIC mp_obj_t robotics_SpikeBase_steering_move_for_degrees(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
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
    pbio_actuation_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Convert steering to tank drive
    int32_t speed_left;
    int32_t speed_right;
    pb_assert(pbio_spikebase_steering_to_tank(speed, steering, &speed_left, &speed_right));

    // Run tank maneuver.
    pb_assert(pbio_spikebase_drive_angle(self->db, speed_left, speed_right, angle, then));

    if (mp_obj_is_true(wait_in)) {
        wait_for_completion_drivebase(self->db);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_steering_move_for_degrees_obj, 1, robotics_SpikeBase_steering_move_for_degrees);

// pybricks.robotics.SpikeBase.drive_forever
STATIC mp_obj_t robotics_SpikeBase_drive_forever(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        robotics_SpikeBase_obj_t, self,
        PB_ARG_REQUIRED(speed_left),
        PB_ARG_REQUIRED(speed_right));

    // Get wheel diameter and axle track dimensions
    mp_int_t speed_left = pb_obj_get_int(speed_left_in);
    mp_int_t speed_right = pb_obj_get_int(speed_right_in);

    pb_assert(pbio_spikebase_drive_forever(self->db, speed_left, speed_right));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(robotics_SpikeBase_drive_forever_obj, 1, robotics_SpikeBase_drive_forever);

// pybricks.robotics.SpikeBase.stop
STATIC mp_obj_t robotics_SpikeBase_stop(mp_obj_t self_in) {
    robotics_SpikeBase_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_drivebase_stop(self->db, PBIO_ACTUATION_COAST));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(robotics_SpikeBase_stop_obj, robotics_SpikeBase_stop);

STATIC const mp_rom_map_elem_t attribute_table[] = {
    PB_DEFINE_CONST_ATTR_RO(robotics_SpikeBase_obj_t, MP_QSTR_left, left),
    PB_DEFINE_CONST_ATTR_RO(robotics_SpikeBase_obj_t, MP_QSTR_right, right),
    #if PYBRICKS_PY_COMMON_CONTROL
    PB_DEFINE_CONST_ATTR_RO(robotics_SpikeBase_obj_t, MP_QSTR_heading_control, heading_control),
    PB_DEFINE_CONST_ATTR_RO(robotics_SpikeBase_obj_t, MP_QSTR_distance_control, distance_control),
    #endif
};
STATIC MP_DEFINE_CONST_DICT(attribute_dict, attribute_table);

// dir(pybricks.robotics.SpikeBase)
STATIC const mp_rom_map_elem_t robotics_SpikeBase_locals_dict_table[] = {
    PB_ATTRIBUTE_TABLE(attribute_dict),
    { MP_ROM_QSTR(MP_QSTR_tank_move_for_degrees),     MP_ROM_PTR(&robotics_SpikeBase_tank_move_for_degrees_obj)     },
    { MP_ROM_QSTR(MP_QSTR_tank_move_for_time),        MP_ROM_PTR(&robotics_SpikeBase_tank_move_for_time_obj)        },
    { MP_ROM_QSTR(MP_QSTR_tank_move_forever),         MP_ROM_PTR(&robotics_SpikeBase_drive_forever_obj)             },
    { MP_ROM_QSTR(MP_QSTR_steering_move_for_degrees), MP_ROM_PTR(&robotics_SpikeBase_steering_move_for_degrees_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                      MP_ROM_PTR(&robotics_SpikeBase_stop_obj)                      },
};
STATIC MP_DEFINE_CONST_DICT(robotics_SpikeBase_locals_dict, robotics_SpikeBase_locals_dict_table);

// type(pybricks.robotics.SpikeBase)
const mp_obj_type_t pb_type_spikebase = {
    { &mp_type_type },
    .name = MP_QSTR_SpikeBase,
    .make_new = robotics_SpikeBase_make_new,
    .locals_dict = (mp_obj_dict_t *)&robotics_SpikeBase_locals_dict,
};

#endif // PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS && (PYBRICKS_HUB_PRIMEHUB || PYBRICKS_HUB_ESSENTIALHUB)
