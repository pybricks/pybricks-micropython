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
} robotics_SpikeBase_obj_t;

// pybricks.robotics.SpikeBase.__init__
STATIC mp_obj_t robotics_SpikeBase_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port_left),
        PB_ARG_REQUIRED(port_right));

    robotics_SpikeBase_obj_t *self = m_new_obj(robotics_SpikeBase_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    pbio_error_t err;

    // Get left servo
    pbio_servo_t *srv_left;
    pb_assert(pbio_servo_get_servo(pb_type_enum_get_value(port_left_in, &pb_enum_type_Port), &srv_left));
    while ((err = pbio_servo_setup(srv_left, PBIO_DIRECTION_CLOCKWISE, fix16_one, false)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }
    pb_assert(err);

    // Get right servo
    pbio_servo_t *srv_right;
    pb_assert(pbio_servo_get_servo(pb_type_enum_get_value(port_right_in, &pb_enum_type_Port), &srv_right));
    while ((err = pbio_servo_setup(srv_right, PBIO_DIRECTION_CLOCKWISE, fix16_one, false)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }
    pb_assert(err);

    // Create drivebase
    pb_assert(pbio_drivebase_get_spikebase(&self->db, srv_left, srv_right));

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

// dir(pybricks.robotics.SpikeBase)
STATIC const mp_rom_map_elem_t robotics_SpikeBase_locals_dict_table[] = {
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
