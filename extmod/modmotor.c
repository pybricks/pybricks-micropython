// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 David Lechner
// Copyright (c) 2019 LEGO System A/S

#include "py/mpconfig.h"

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

#include <inttypes.h>

#include <pbio/servo.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/obj.h"

#include "modmotor.h"
#include "modlogger.h"
#include "modparameters.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"
#include "pbthread.h"

/* Wait for maneuver to complete */

// Must not be called while pybricks thread lock is held!
STATIC void wait_for_completion(pbio_servo_t *srv) {
    while (srv->state >= PBIO_SERVO_STATE_ANGLE_FOREGROUND && srv->state < PBIO_SERVO_STATE_CLAIMED) {
        mp_hal_delay_ms(10);
    }
    if (srv->state == PBIO_SERVO_STATE_ERRORED) {
        pb_assert(PBIO_ERROR_IO);
    }
}

STATIC mp_obj_t motor_Motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_ENUM(direction, pb_const_clockwise),
        PB_ARG_DEFAULT_NONE(gears)
    );

    motor_Motor_obj_t *self = m_new_obj(motor_Motor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    // Default gear ratio
    fix16_t gear_ratio = F16C(1, 0);

    // Parse gear argument of the form [[12, 20, 36], [20, 40]] or [12, 20, 36]
    if (gears != mp_const_none) {
        // Unpack the main list
        mp_obj_t *trains, *gear_list;
        size_t n_trains, n_gears;
        mp_obj_get_array(gears, &n_trains, &trains);

        // If the first and last element is an integer, assume the user gave just one list of gears, i.e. [12, 20, 36]
        bool is_one_train = MP_OBJ_IS_SMALL_INT(trains[0]) && MP_OBJ_IS_SMALL_INT(trains[n_trains-1]);
        // This means we don't have a list of gear trains, but just one gear train with a given number of gears
        if (is_one_train) {
            n_gears = n_trains;
            gear_list = trains;
            n_trains = 1;
        }

        // Iterate through each of the n_trains lists
        for (int16_t train = 0; train < n_trains; train++) {
            // Unless we have just one list of gears, unpack the list of gears for this train
            if (!is_one_train) {
                mp_obj_get_array(trains[train], &n_gears, &gear_list);
            }
            // For this gear train, compute the ratio from the first and last gear
            fix16_t first_gear = fix16_from_int(mp_obj_get_int(gear_list[0]));
            fix16_t last_gear = fix16_from_int(mp_obj_get_int(gear_list[n_gears-1]));
            if (first_gear < 1 || last_gear < 1) {
                pb_assert(PBIO_ERROR_INVALID_ARG);
            }
            // Include the ratio of this train in the overall gear train
            gear_ratio = fix16_div(fix16_mul(gear_ratio, last_gear), first_gear);
        }
    }
    // Configure the encoded motor with the selected arguments at pbio level
    mp_int_t port_arg = pb_type_enum_get_value(port, &pb_enum_type_Port);
    pbio_direction_t direction_arg = pb_type_enum_get_value(direction, &pb_enum_type_Direction);

    pb_thread_enter();
    pb_assert(pbio_servo_get(port_arg, &self->srv, direction_arg, gear_ratio));
    pb_thread_exit();

    // Create an instance of the Logger class
    self->logger = logger_obj_make_new(&self->srv->log);

    return MP_OBJ_FROM_PTR(self);
}

void motor_Motor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_error_t err;

    pb_thread_enter();
    char counts_per_degree_str[13];
    char gear_ratio_str[13];
    err = pbio_servo_get_gear_settings(self->srv, gear_ratio_str, counts_per_degree_str);
    if (err != PBIO_SUCCESS) {
        pb_thread_exit();
        pb_assert(err);
    }

    pbio_direction_t direction;
    int32_t stall_duty, duty_offset;
    err = pbio_hbridge_get_settings(self->srv->hbridge, &direction, &stall_duty, &duty_offset);
    if (err != PBIO_SUCCESS) {
        pb_thread_exit();
        pb_assert(err);
    }

    int32_t max_speed, acceleration;
    err = pbio_servo_get_run_settings(self->srv, &max_speed, &acceleration);
    if (err != PBIO_SUCCESS) {
        pb_thread_exit();
        pb_assert(err);
    }

    int16_t pid_kp, pid_ki, pid_kd;
    int32_t tight_loop_time, stall_time;
    int32_t position_tolerance, speed_tolerance, stall_speed_limit;
    err = pbio_servo_get_pid_settings(
        self->srv,
        &pid_kp, &pid_ki, &pid_kd,
        &tight_loop_time, &position_tolerance,
        &speed_tolerance, &stall_speed_limit, &stall_time);
    if (err != PBIO_SUCCESS) {
        pb_thread_exit();
        pb_assert(err);
    }

    pb_thread_exit();

    char settings_string[512];

    snprintf(settings_string, sizeof(settings_string),
        "Motor properties:\n"
        "------------------------\n"
        "Port\t\t %c\n"
        "Direction\t %s\n"
        "Counts per unit\t %s\n"
        "Gear ratio\t %s\n"
        "\nRun settings:\n"
        "------------------------\n"
        "Max speed\t %" PRId32 "\n"
        "Acceleration\t %" PRId32 "\n"
        "\nDC settings:\n"
        "------------------------\n"
        "Duty limit\t %" PRId32 "\n"
        "Duty offset\t %" PRId32 "\n"
        "\nPID settings:\n"
        "------------------------\n"
        "kp\t\t %" PRId16 "\n"
        "ki\t\t %" PRId16 "\n"
        "kd\t\t %" PRId16 "\n"
        "Tight Loop\t %" PRId32 "\n"
        "Angle tolerance\t %" PRId32 "\n"
        "Speed tolerance\t %" PRId32 "\n"
        "Stall speed\t %" PRId32 "\n"
        "Stall time\t %" PRId32,
        self->srv->port,
        direction == PBIO_DIRECTION_CLOCKWISE ? "clockwise" : "counterclockwise",
        counts_per_degree_str,
        gear_ratio_str,
        // Print run settings
        max_speed,
        acceleration,
        // Print DC settings
        stall_duty,
        duty_offset,
        // Print PID settings
        pid_kp,
        pid_ki,
        pid_kd,
        tight_loop_time,
        position_tolerance,
        speed_tolerance,
        stall_speed_limit,
        stall_time
    );

    mp_printf(print, "%s\n", settings_string);
}


STATIC mp_obj_t motor_Motor_duty(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_REQUIRED(duty)
    );

    mp_int_t duty_cycle = pb_obj_get_int(duty);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_servo_set_duty_cycle(self->srv, duty_cycle);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_duty_obj, 0, motor_Motor_duty);

STATIC mp_obj_t motor_Motor_angle(mp_obj_t self_in) {
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t angle;
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_tacho_get_angle(self->srv->tacho, &angle);
    pb_thread_exit();

    pb_assert(err);
    return mp_obj_new_int(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_angle_obj, motor_Motor_angle);

STATIC mp_obj_t motor_Motor_stalled(mp_obj_t self_in) {
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bool stalled;
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_servo_is_stalled(self->srv, &stalled);
    pb_thread_exit();

    pb_assert(err);

    return mp_obj_new_bool(stalled);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_stalled_obj, motor_Motor_stalled);

STATIC mp_obj_t motor_Motor_reset_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_DEFAULT_NONE(angle)
    );

    // If no angle argument is given, reset to the absolute value
    bool reset_to_abs = angle == mp_const_none;

    // Otherwise reset to the given angle
    mp_int_t reset_angle = reset_to_abs ? 0 : pb_obj_get_int(angle);

    // Set the new angle
    pbio_error_t err;
    pb_thread_enter();
    err = pbio_servo_reset_angle(self->srv, reset_angle, reset_to_abs);
    pb_thread_exit();

    // Assert that no errors were raised
    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_reset_angle_obj, 0, motor_Motor_reset_angle);

STATIC mp_obj_t motor_Motor_speed(mp_obj_t self_in) {
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t speed;
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_tacho_get_angular_rate(self->srv->tacho, &speed);
    pb_thread_exit();

    pb_assert(err);

    return mp_obj_new_int(speed);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_speed_obj, motor_Motor_speed);

STATIC mp_obj_t motor_Motor_run(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed)
    );

    mp_int_t speed_arg = pb_obj_get_int(speed);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_servo_run(self->srv, speed_arg);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_run_obj, 0, motor_Motor_run);

STATIC mp_obj_t motor_Motor_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_DEFAULT_ENUM(stop_type, pb_const_coast)
    );
    pbio_actuation_t after_stop = pb_type_enum_get_value(stop_type, &pb_enum_type_Stop);
    pbio_error_t err;

    pb_thread_enter();

    // TODO: raise PBIO_ERROR_INVALID_ARG for Stop.HOLD in case of dc motor

    // Call pbio with parsed user/default arguments
    err = pbio_servo_stop(self->srv, after_stop);

    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_stop_obj, 0, motor_Motor_stop);

STATIC mp_obj_t motor_Motor_run_time(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(time),
        PB_ARG_DEFAULT_ENUM(stop_type, pb_const_coast),
        PB_ARG_DEFAULT_TRUE(wait)
    );

    mp_int_t speed_arg = pb_obj_get_int(speed);
    mp_int_t time_arg = pb_obj_get_int(time);
    pbio_actuation_t after_stop = pb_type_enum_get_value(stop_type, &pb_enum_type_Stop);
    bool foreground = mp_obj_is_true(wait);
    pbio_error_t err;

    pb_thread_enter();
    // Call pbio with parsed user/default arguments
    err = pbio_servo_run_time(self->srv, speed_arg, time_arg, after_stop, foreground);
    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(self->srv);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_run_time_obj, 0, motor_Motor_run_time);

STATIC mp_obj_t motor_Motor_run_until_stalled(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_DEFAULT_ENUM(stop_type, pb_const_coast),
        PB_ARG_DEFAULT_INT(duty_limit, 100)
    );

    mp_int_t speed_arg = pb_obj_get_int(speed);
    pbio_actuation_t after_stop = pb_type_enum_get_value(stop_type, &pb_enum_type_Stop);

    int32_t temporary_stall_duty = 100;
    int32_t old_stall_duty;
    pbio_direction_t direction;
    int32_t old_duty_offset;
    pbio_error_t err;

    bool override_duty_limit = n_args > 3;

    if (override_duty_limit) {
        temporary_stall_duty = pb_obj_get_int(duty_limit);
    }

    pb_thread_enter();

    if (override_duty_limit) {
        pbio_hbridge_get_settings(self->srv->hbridge, &direction, &old_stall_duty, &old_duty_offset);
        pbio_hbridge_set_settings(self->srv->hbridge, temporary_stall_duty, old_duty_offset);
    }

    // Call pbio with parsed user/default arguments
    err = pbio_servo_run_until_stalled(self->srv, speed_arg, after_stop);

    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(self->srv);

    pb_thread_enter();

    // Read the angle upon completion of the stall maneuver
    int32_t stall_point;
    pbio_tacho_get_angle(self->srv->tacho, &stall_point);

    if (override_duty_limit) {
        // Return stall settings to old values if they were changed
        pbio_hbridge_set_settings(self->srv->hbridge, old_stall_duty, old_duty_offset);
    }

    pb_thread_exit();

    // Return angle at which the motor stalled
    return mp_obj_new_int(stall_point);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_run_until_stalled_obj, 0, motor_Motor_run_until_stalled);

STATIC mp_obj_t motor_Motor_run_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(rotation_angle),
        PB_ARG_DEFAULT_ENUM(stop_type, pb_const_coast),
        PB_ARG_DEFAULT_TRUE(wait)
    );

    mp_int_t speed_arg = pb_obj_get_int(speed);
    mp_int_t angle_arg = pb_obj_get_int(rotation_angle);
    pbio_actuation_t after_stop = pb_type_enum_get_value(stop_type, &pb_enum_type_Stop);
    bool foreground = mp_obj_is_true(wait);
    pbio_error_t err;

    pb_thread_enter();
    // Call pbio with parsed user/default arguments
    err = pbio_servo_run_angle(self->srv, speed_arg, angle_arg, after_stop, foreground);
    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(self->srv);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_run_angle_obj, 0, motor_Motor_run_angle);

STATIC mp_obj_t motor_Motor_run_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(target_angle),
        PB_ARG_DEFAULT_ENUM(stop_type, pb_const_coast),
        PB_ARG_DEFAULT_TRUE(wait)
    );

    mp_int_t speed_arg = pb_obj_get_int(speed);
    mp_int_t angle_arg = pb_obj_get_int(target_angle);
    pbio_actuation_t after_stop = pb_type_enum_get_value(stop_type, &pb_enum_type_Stop);
    bool foreground = mp_obj_is_true(wait);
    pbio_error_t err;

    // Call pbio with parsed user/default arguments
    pb_thread_enter();
    err = pbio_servo_run_target(self->srv, speed_arg, angle_arg, after_stop, foreground);
    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(self->srv);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_run_target_obj, 0, motor_Motor_run_target);

STATIC mp_obj_t motor_Motor_track_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_REQUIRED(target_angle)
    );

    mp_int_t target = pb_obj_get_int(target_angle);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_servo_track_target(self->srv, target);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_track_target_obj, 0, motor_Motor_track_target);

STATIC mp_obj_t motor_Motor_set_run_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_DEFAULT_NONE(max_speed),
        PB_ARG_DEFAULT_NONE(acceleration)
    );

    // Load original values
    pbio_error_t err;
    int32_t max_speed_val;
    int32_t acceleration_val;

    pb_thread_enter();
    err = pbio_servo_get_run_settings(self->srv, &max_speed_val, &acceleration_val);
    pb_thread_exit();

    // Set values if given by the user
    max_speed_val = pb_obj_get_default_int(max_speed, max_speed_val);
    acceleration_val = pb_obj_get_default_int(acceleration, acceleration_val);

    // Write resulting values
    pb_thread_enter();
    err = pbio_servo_set_run_settings(self->srv, max_speed_val, acceleration_val);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_set_run_settings_obj, 0, motor_Motor_set_run_settings);

STATIC mp_obj_t motor_Motor_set_dc_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_DEFAULT_NONE(duty_limit),
        PB_ARG_DEFAULT_NONE(duty_offset)
    );

    // Load original values
    pbio_error_t err;
    pbio_direction_t direction;
    int32_t stall_torque_limit_pct;
    int32_t duty_offset_pct;

    pb_thread_enter();
    err = pbio_hbridge_get_settings(self->srv->hbridge, &direction, &stall_torque_limit_pct, &duty_offset_pct);
    pb_thread_exit();

    // Set values if given by the user
    stall_torque_limit_pct = pb_obj_get_default_int(duty_limit, stall_torque_limit_pct);
    duty_offset_pct = pb_obj_get_default_int(duty_offset, duty_offset_pct);

    // Write resulting values
    pb_thread_enter();
    err = pbio_hbridge_set_settings(self->srv->hbridge, stall_torque_limit_pct, duty_offset_pct);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_set_dc_settings_obj, 0, motor_Motor_set_dc_settings);

STATIC mp_obj_t motor_Motor_set_pid_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        motor_Motor_obj_t, self,
        PB_ARG_DEFAULT_NONE(kp),
        PB_ARG_DEFAULT_NONE(ki),
        PB_ARG_DEFAULT_NONE(kd),
        PB_ARG_DEFAULT_NONE(tight_loop_limit),
        PB_ARG_DEFAULT_NONE(angle_tolerance),
        PB_ARG_DEFAULT_NONE(speed_tolerance),
        PB_ARG_DEFAULT_NONE(stall_speed),
        PB_ARG_DEFAULT_NONE(stall_time)
    );

    // Load original values
    pbio_error_t err;
    int16_t kp_val;
    int16_t ki_val;
    int16_t kd_val;
    int32_t loop_time_val;
    int32_t pos_tolerance_val;
    int32_t speed_tolerance_val;
    int32_t stall_speed_limit_val;
    int32_t stall_time_val;

    pb_thread_enter();
    err = pbio_servo_get_pid_settings(self->srv, &kp_val, &ki_val, &kd_val, &loop_time_val, &pos_tolerance_val, &speed_tolerance_val, &stall_speed_limit_val, &stall_time_val);
    pb_thread_exit();

    // Set values if given by the user
    kp_val = pb_obj_get_default_int(kp, kp_val);
    ki_val = pb_obj_get_default_int(ki, ki_val);
    kd_val = pb_obj_get_default_int(kd, kd_val);
    loop_time_val = pb_obj_get_default_int(tight_loop_limit, loop_time_val);
    pos_tolerance_val = pb_obj_get_default_int(angle_tolerance, pos_tolerance_val);
    speed_tolerance_val = pb_obj_get_default_int(speed_tolerance, speed_tolerance_val);
    stall_speed_limit_val = pb_obj_get_default_int(stall_speed, stall_speed_limit_val);
    stall_time_val = pb_obj_get_default_int(stall_time, stall_time_val);

    // Write resulting values
    pb_thread_enter();
    err = pbio_servo_set_pid_settings(self->srv, kp_val, ki_val, kd_val, loop_time_val, pos_tolerance_val, speed_tolerance_val, stall_speed_limit_val, stall_time_val);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(motor_Motor_set_pid_settings_obj, 0, motor_Motor_set_pid_settings);

STATIC mp_obj_t motor_Motor_trajectory(mp_obj_t self_in) {
    motor_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_control_trajectory_t trajectory;

    mp_obj_t parms[12];

    pb_thread_enter();
    bool active = self->srv->state > PBIO_SERVO_STATE_ERRORED;
    trajectory = self->srv->control.trajectory;
    pb_thread_exit();

    if (active) {
        parms[0] = mp_obj_new_int((trajectory.t0-trajectory.t0)/1000);
        parms[1] = mp_obj_new_int((trajectory.t1-trajectory.t0)/1000);
        parms[2] = mp_obj_new_int((trajectory.t2-trajectory.t0)/1000);
        parms[3] = mp_obj_new_int((trajectory.t3-trajectory.t0)/1000);
        parms[4] = mp_obj_new_int(trajectory.th0);
        parms[5] = mp_obj_new_int(trajectory.th1);
        parms[6] = mp_obj_new_int(trajectory.th2);
        parms[7] = mp_obj_new_int(trajectory.th3);
        parms[8] = mp_obj_new_int(trajectory.w0);
        parms[9] = mp_obj_new_int(trajectory.w1);
        parms[10] = mp_obj_new_int(trajectory.a0);
        parms[11] = mp_obj_new_int(trajectory.a2);
        return mp_obj_new_tuple(12, parms);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_trajectory_obj, motor_Motor_trajectory);

/*
Motor Class tables
*/

STATIC const mp_rom_map_elem_t motor_Motor_locals_dict_table[] = {
    //
    // Methods common to DC motors and encoded motors
    //
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&motor_Motor_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_dc), MP_ROM_PTR(&motor_Motor_duty_obj) },
    //
    // Methods specific to encoded motors
    //
    { MP_ROM_QSTR(MP_QSTR_set_run_settings), MP_ROM_PTR(&motor_Motor_set_run_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pid_settings), MP_ROM_PTR(&motor_Motor_set_pid_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_dc_settings), MP_ROM_PTR(&motor_Motor_set_dc_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&motor_Motor_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_speed), MP_ROM_PTR(&motor_Motor_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&motor_Motor_reset_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_stalled), MP_ROM_PTR(&motor_Motor_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&motor_Motor_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_time), MP_ROM_PTR(&motor_Motor_run_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_until_stalled), MP_ROM_PTR(&motor_Motor_run_until_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_angle), MP_ROM_PTR(&motor_Motor_run_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_target), MP_ROM_PTR(&motor_Motor_run_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_track_target), MP_ROM_PTR(&motor_Motor_track_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_log), MP_ROM_ATTRIBUTE_OFFSET(motor_Motor_obj_t, logger) },
    { MP_ROM_QSTR(MP_QSTR_trajectory), MP_ROM_PTR(&motor_Motor_trajectory_obj) },
};
MP_DEFINE_CONST_DICT(motor_Motor_locals_dict, motor_Motor_locals_dict_table);

const mp_obj_type_t motor_Motor_type = {
    { &mp_type_type },
    .name = MP_QSTR_Motor,
#ifndef PBDRV_CONFIG_HUB_MOVEHUB
    .print = motor_Motor_print,
#endif
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_Motor_locals_dict,
};
#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER
