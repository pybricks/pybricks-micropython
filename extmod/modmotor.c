/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Laurens Valk
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <pbio/motorcontrol.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/builtin.h"

#include "modmotor.h"
#include "pberror.h"
#include "pbid.h"
#include "pbobj.h"
#include "pbthread.h"


/* Wait for maneuver to complete */

// Must not be called while pybricks thread lock is held!
STATIC void wait_for_completion(pbio_port_t port, pbio_motor_run_t runtype) {
    if (runtype == PBIO_MOTOR_RUN_FOREGROUND) {
        while (motor_control_active[PORT_TO_IDX(port)] >= PBIO_MOTOR_CONTROL_RUNNING) {
            mp_hal_delay_ms(10);
        }
    };
}

mp_obj_t motor_Motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args){
    // Load self and determine port
    mp_arg_check_num(n_args, n_kw, 1, 3, false);
    motor_Motor_obj_t *self = m_new_obj(motor_Motor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    self->port = mp_obj_get_int(args[0]);
    // Configure direction or set to default
    int8_t direction = (n_args > 1) ? mp_obj_get_int(args[1]) : PBIO_MOTOR_DIR_CLOCKWISE;
    pbio_error_t err;

    // Default gear ratio
    float_t gear_ratio = 1.0;

    // Parse gear argument of the form [[12, 20, 36], [20, 40]] or [12, 20, 36]
    if (n_args == 3) {
        // Unpack the main list
        mp_obj_t *trains, *gears;
        size_t n_trains, n_gears;
        mp_obj_get_array(args[2], &n_trains, &trains);

        // If the first and last element is an integer, assume the user gave just one list of gears, i.e. [12, 20, 36]
        bool is_one_train = MP_OBJ_IS_SMALL_INT(trains[0]) && MP_OBJ_IS_SMALL_INT(trains[n_trains-1]);
        // This means we don't have a list of gear trains, but just one gear train with a given number of gears
        if (is_one_train) {
            n_gears = n_trains;
            gears = trains;
            n_trains = 1;
        }

        // Iterate through each of the n_trains lists
        for (int16_t train = 0; train < n_trains; train++) {
            // Unless we have just one list of gears, unpack the list of gears for this train
            if (!is_one_train) {
                mp_obj_get_array(trains[train], &n_gears, &gears);
            }
            // For this gear train, compute the ratio from the first and last gear
            int16_t first_gear = mp_obj_get_int(gears[0]);
            int16_t last_gear = mp_obj_get_int(gears[n_gears-1]);
            if (first_gear < 1 || last_gear < 1) {
                pb_assert(PBIO_ERROR_INVALID_ARG);
            }
            // Include the ratio of this train in the overall gear train
            gear_ratio = (gear_ratio*last_gear)/first_gear;
        }
    }
    // Configure the encoded motor with the selected arguments at pbio level
    pb_thread_enter();
    err = pbio_motor_setup(self->port, direction, gear_ratio);
    pb_thread_exit();

    pb_assert(err);

    return MP_OBJ_FROM_PTR(self);
}

void motor_Motor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind){
    pbio_port_t port = get_port(self_in);
    char dcmotor_settings_string[MAX_DCMOTOR_SETTINGS_STR_LENGTH];

    pb_thread_enter();

    pbio_dcmotor_print_settings(port, dcmotor_settings_string);
    if (pbio_encmotor_has_encoder(port)) {
        char encmotor_settings_string[MAX_ENCMOTOR_SETTINGS_STR_LENGTH];
        pbio_encmotor_print_settings(port, encmotor_settings_string);
        mp_printf(print, "%s\n%s", dcmotor_settings_string, encmotor_settings_string);
    }
    else {
        mp_printf(print, "%s", dcmotor_settings_string);
    }

    pb_thread_exit();
}


STATIC mp_obj_t motor_Motor_duty(mp_obj_t self_in, mp_obj_t duty){
    pbio_port_t port = get_port(self_in);
    float_t duty_cycle = mp_obj_get_num(duty);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_dcmotor_set_duty_cycle(port, duty_cycle);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_Motor_duty_obj, motor_Motor_duty);

STATIC mp_obj_t motor_Motor_angle(mp_obj_t self_in) {
    pbio_port_t port = get_port(self_in);
    int32_t angle;
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_get_angle(port, &angle);
    pb_thread_exit();

    pb_assert(err);

    return mp_obj_new_int(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_angle_obj, motor_Motor_angle);

STATIC mp_obj_t motor_Motor_stalled(mp_obj_t self_in) {
    pbio_port_t port = get_port(self_in);
    bool stalled;
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_is_stalled(port, &stalled);
    pb_thread_exit();

    pb_assert(err);

    return mp_obj_new_bool(stalled);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_stalled_obj, motor_Motor_stalled);

STATIC mp_obj_t motor_Motor_reset_angle(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);
    int32_t reset_angle = n_args > 1 ? mp_obj_get_num(args[1]) : 0;
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_reset_angle(port, reset_angle);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_reset_angle_obj, 1, 2, motor_Motor_reset_angle);

STATIC mp_obj_t motor_Motor_speed(mp_obj_t self_in) {
    pbio_port_t port = get_port(self_in);
    int32_t speed;
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_get_angular_rate(port, &speed);
    pb_thread_exit();

    pb_assert(err);

    return mp_obj_new_int(speed);
}
MP_DEFINE_CONST_FUN_OBJ_1(motor_Motor_speed_obj, motor_Motor_speed);

STATIC mp_obj_t motor_Motor_run(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);
    int32_t speed = mp_obj_get_num(args[1]);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_run(port, speed);
    pb_thread_exit();

    pb_assert(err);
    mp_hal_delay_ms(15);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_obj, 2, 3, motor_Motor_run);

STATIC mp_obj_t motor_Motor_stop(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    pbio_motor_after_stop_t after_stop = n_args > 1 ? mp_obj_get_int(args[1]) : PBIO_MOTOR_STOP_COAST;
    pbio_error_t err;

    pb_thread_enter();

    if (!pbio_encmotor_has_encoder(port) && after_stop == PBIO_MOTOR_STOP_HOLD) {
        pb_thread_exit();
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    // Call pbio with parsed user/default arguments
    err = pbio_encmotor_stop(port, after_stop);

    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_stop_obj, 1, 2, motor_Motor_stop);

STATIC mp_obj_t motor_Motor_run_time(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    int32_t speed = mp_obj_get_num(args[1]);
    int32_t duration = mp_obj_get_num(args[2]);
    pbio_motor_after_stop_t after_stop = n_args > 3 ? mp_obj_get_int(args[3]) : PBIO_MOTOR_STOP_COAST;
    pbio_motor_run_t runtype           = n_args > 4 ? mp_obj_get_int(args[4]) : PBIO_MOTOR_RUN_FOREGROUND;
    pbio_error_t err;

    pb_thread_enter();
    // Call pbio with parsed user/default arguments
    err = pbio_encmotor_run_time(port, speed, duration, after_stop);
    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(port, runtype);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_time_obj, 3, 5, motor_Motor_run_time);

STATIC mp_obj_t motor_Motor_run_until_stalled(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    int32_t speed = mp_obj_get_num(args[1]);
    pbio_motor_after_stop_t after_stop = n_args > 2 ? mp_obj_get_int(args[2]) : PBIO_MOTOR_STOP_COAST;
    pbio_error_t err;

    // Save old settings
    int32_t old_stall_duty;
    int32_t old_duty_offset;

    pb_thread_enter();

    pbio_encmotor_get_dc_settings(port, &old_stall_duty, &old_duty_offset);

    pb_thread_exit();

    int32_t temporary_stall_duty  = n_args > 3 ? mp_obj_get_num(args[3]) : old_stall_duty;

    pb_thread_enter();

    // Set temporary settings supplied by user for this maneuver
    pbio_encmotor_set_dc_settings(port, temporary_stall_duty, old_duty_offset);

    // Call pbio with parsed user/default arguments
    err = pbio_encmotor_run_until_stalled(port, speed, after_stop);

    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(port, PBIO_MOTOR_RUN_FOREGROUND);

    pb_thread_enter();

    // Read the angle upon completion of the stall maneuver
    int32_t stall_point;
    pbio_encmotor_get_angle(port, &stall_point);

    // Return stall settings to old values
    pbio_encmotor_set_dc_settings(port, old_stall_duty, old_duty_offset);

    pb_thread_exit();

    // Return angle at which the motor stalled
    return mp_obj_new_int(stall_point);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_until_stalled_obj, 2, 6, motor_Motor_run_until_stalled);

STATIC mp_obj_t motor_Motor_run_angle(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    int32_t speed = mp_obj_get_num(args[1]);
    int32_t angle = mp_obj_get_num(args[2]);
    pbio_motor_after_stop_t after_stop = n_args > 3 ? mp_obj_get_int(args[3]) : PBIO_MOTOR_STOP_COAST;
    pbio_motor_run_t runtype           = n_args > 4 ? mp_obj_get_int(args[4]) : PBIO_MOTOR_RUN_FOREGROUND;
    pbio_error_t err;

    pb_thread_enter();
    // Call pbio with parsed user/default arguments
    err = pbio_encmotor_run_angle(port, speed, angle, after_stop);
    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(port, runtype);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_angle_obj, 3, 5, motor_Motor_run_angle);

STATIC mp_obj_t motor_Motor_run_target(size_t n_args, const mp_obj_t *args){
    // Parse arguments and/or set default optional arguments
    pbio_port_t port = get_port(args[0]);
    int32_t speed = mp_obj_get_num(args[1]);
    int32_t target = mp_obj_get_num(args[2]);
    pbio_motor_after_stop_t after_stop = n_args > 3 ? mp_obj_get_int(args[3]) : PBIO_MOTOR_STOP_COAST;
    pbio_motor_run_t runtype           = n_args > 4 ? mp_obj_get_int(args[4]) : PBIO_MOTOR_RUN_FOREGROUND;
    pbio_error_t err;

    // Call pbio with parsed user/default arguments
    pb_thread_enter();
    err = pbio_encmotor_run_target(port, speed, target, after_stop);
    pb_thread_exit();

    pb_assert(err);
    wait_for_completion(port, runtype);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_run_target_obj, 3, 5, motor_Motor_run_target);

STATIC mp_obj_t motor_Motor_track_target(mp_obj_t self_in, mp_obj_t target) {
    pbio_port_t port = get_port(self_in);
    int32_t target_ = mp_obj_get_num(target);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_track_target(port, target_);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(motor_Motor_track_target_obj, motor_Motor_track_target);

STATIC mp_obj_t motor_Motor_set_run_settings(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);
    int32_t max_speed = mp_obj_get_num(args[1]);
    int32_t acceleration = mp_obj_get_num(args[2]);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_set_run_settings(port, max_speed, acceleration);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_set_run_settings_obj, 3, 3, motor_Motor_set_run_settings);

STATIC mp_obj_t motor_Motor_set_dc_settings(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);
    int32_t stall_torque_limit_pct = mp_obj_get_num(args[1]);
    int32_t duty_offset_pct = mp_obj_get_num(args[2]);
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_set_dc_settings(port, stall_torque_limit_pct, duty_offset_pct);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_set_dc_settings_obj, 3, 3, motor_Motor_set_dc_settings);

STATIC mp_obj_t motor_Motor_set_pid_settings(size_t n_args, const mp_obj_t *args){
    pbio_port_t port = get_port(args[0]);
    int16_t kp = mp_obj_get_num(args[1]);
    int16_t ki = mp_obj_get_num(args[2]);
    int16_t kd = mp_obj_get_num(args[3]);
    int16_t loop_time = mp_obj_get_num(args[4]);
    int16_t pos_tolerance = mp_obj_get_num(args[5]);
    int16_t speed_tolerance = mp_obj_get_num(args[6]);
    int16_t stall_speed_limit = mp_obj_get_num(args[7]);
    int16_t stall_time = mp_obj_get_num(args[8]);    
    pbio_error_t err;

    pb_thread_enter();
    err = pbio_encmotor_set_pid_settings(port, kp, ki, kd, loop_time, pos_tolerance, speed_tolerance, stall_speed_limit, stall_time);
    pb_thread_exit();

    pb_assert(err);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(motor_Motor_set_pid_settings_obj, 9, 9, motor_Motor_set_pid_settings);

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
};
MP_DEFINE_CONST_DICT(motor_Motor_locals_dict, motor_Motor_locals_dict_table);

#if PBIO_CONFIG_ENABLE_MOTORS
const mp_obj_type_t motor_Motor_type = {
    { &mp_type_type },
    .name = MP_QSTR_Motor,
    .print = motor_Motor_print,
    .make_new = motor_Motor_make_new,
    .locals_dict = (mp_obj_dict_t*)&motor_Motor_locals_dict,
};
#endif //PBIO_CONFIG_ENABLE_MOTORS
