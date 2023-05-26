// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_MOTORS

#include <pbio/battery.h>

#include <pbio/dcmotor.h>
#include <pbio/int_math.h>
#include <pbio/servo.h>

#include "py/mphal.h"
#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_awaitable.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

// Gets the number of millidegrees of the motor, for each whole degree
// of rotation at the gear train output. For example, if the gear train
// slows the motor down using a 12 teeth and a 36 teeth gear, the result
// should be (36 / 12) * 1000 = 3000.
static int32_t get_gear_ratio(mp_obj_t gears_in) {

    // No gears means gear ratio is one.
    if (gears_in == mp_const_none) {
        return 1000;
    }

    // Unpack the main list of multiple gear trains.
    mp_obj_t *trains, *gear_list;
    size_t n_trains, n_gears;
    mp_obj_get_array(gears_in, &n_trains, &trains);

    // Parse gear argument of the form [[12, 20, 36], [20, 40]]
    int32_t first_gear_product = 1;
    int32_t last_gear_product = 1;

    // Check type of gear train given.
    if (mp_obj_is_int(trains[0]) && mp_obj_is_int(trains[n_trains - 1])) {
        // If the first and last element is an integer, assume just one list of
        // gears, e.g. [12, 20, 36]. Take ratio of last and first.
        first_gear_product = mp_obj_get_int(trains[0]);
        last_gear_product = mp_obj_get_int(trains[n_trains - 1]);
    } else {
        // Otherwise, parse gear argument of the form [[12, 20, 36], [20, 40]].
        for (size_t train = 0; train < n_trains; train++) {
            // Unpack the list of gears for this train
            mp_obj_get_array(trains[train], &n_gears, &gear_list);

            first_gear_product *= mp_obj_get_int(gear_list[0]);
            last_gear_product *= mp_obj_get_int(gear_list[n_gears - 1]);
        }
    }

    // Verify the result.
    if (first_gear_product < 1 || last_gear_product < 1) {
        mp_raise_msg(&mp_type_ZeroDivisionError, MP_ERROR_TEXT("Gears must be positive integers."));
    }

    // Return ratio scaled to millidegrees.
    return 1000 * last_gear_product / first_gear_product;
}

// pybricks._common.Motor.__init__
STATIC mp_obj_t common_Motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_OBJ(positive_direction, pb_Direction_CLOCKWISE_obj),
        PB_ARG_DEFAULT_NONE(gears),
        PB_ARG_DEFAULT_TRUE(reset_angle),
        PB_ARG_DEFAULT_NONE(profile));

    // Validate arguments before attempting to set up the motor.
    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_direction_t positive_direction = pb_type_enum_get_value(positive_direction_in, &pb_enum_type_Direction);
    bool reset_angle = mp_obj_is_true(reset_angle_in);
    int32_t precision_profile = pb_obj_get_default_abs_int(profile_in, 0);

    pbio_error_t err;
    pbio_servo_t *srv;

    // Setup motor device and raise error if not connected or ready.
    pb_device_setup_motor(port, true);

    // Parse gears argument to get gear ratio.
    int32_t gear_ratio = get_gear_ratio(gears_in);

    // Get pointer to servo and allow tacho to finish syncing
    while ((err = pbio_servo_get_servo(port, &srv)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }
    pb_assert(err);

    // Set up servo
    pb_assert(pbio_servo_setup(srv, positive_direction, gear_ratio, reset_angle, precision_profile));

    // On success, proceed to create and return the MicroPython object
    common_Motor_obj_t *self = mp_obj_malloc(common_Motor_obj_t, type);
    self->srv = srv;
    self->port = port;

    #if PYBRICKS_PY_COMMON_CONTROL
    // Create an instance of the Control class
    self->control = pb_type_Control_obj_make_new(&self->srv->control);
    #endif

    #if PYBRICKS_PY_COMMON_MOTOR_MODEL
    // Create an instance of the MotorModel class
    self->model = pb_type_MotorModel_obj_make_new(&self->srv->observer);
    #endif

    #if PYBRICKS_PY_COMMON_LOGGER
    // Create an instance of the Logger class
    self->logger = common_Logger_obj_make_new(&self->srv->log, PBIO_SERVO_LOGGER_NUM_COLS);
    #endif

    // List of awaitables associated with this motor. By keeping track,
    // we can cancel them as needed when a new movement is started.
    self->awaitables = mp_obj_new_list(0, NULL);

    return MP_OBJ_FROM_PTR(self);
}


STATIC bool common_Motor_test_completion(mp_obj_t self_in, uint32_t end_time) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Handle I/O exceptions like port unplugged.
    if (!pbio_servo_update_loop_is_running(self->srv)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Get completion state.
    return pbio_control_is_done(&self->srv->control);
}

STATIC void common_Motor_cancel(mp_obj_t self_in) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_servo_stop(self->srv, PBIO_CONTROL_ON_COMPLETION_COAST));
}

// Common awaitable used for most motor methods.
STATIC mp_obj_t await_or_wait(common_Motor_obj_t *self) {
    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(self),
        self->awaitables,
        pb_type_awaitable_end_time_none,
        common_Motor_test_completion,
        pb_type_awaitable_return_none,
        common_Motor_cancel,
        PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
}

// pybricks._common.Motor.angle
STATIC mp_obj_t common_Motor_angle(mp_obj_t self_in) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t angle, speed;
    pb_assert(pbio_servo_get_state_user(self->srv, &angle, &speed));

    return mp_obj_new_int(angle);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Motor_angle_obj, common_Motor_angle);

// pybricks._common.Motor.reset_angle
STATIC mp_obj_t common_Motor_reset_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_DEFAULT_NONE(angle));

    // If no angle argument is given, reset to the absolute value
    bool reset_to_abs = angle_in == mp_const_none;

    // Otherwise reset to the given angle
    mp_int_t reset_angle = reset_to_abs ? 0 : pb_obj_get_int(angle_in);

    // Set the new angle
    pb_assert(pbio_servo_reset_angle(self->srv, reset_angle, reset_to_abs));
    pb_type_awaitable_update_all(self->awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_reset_angle_obj, 1, common_Motor_reset_angle);

// pybricks._common.Motor.speed
STATIC mp_obj_t common_Motor_speed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_DEFAULT_INT(window, 100));

    int32_t speed;
    pb_assert(pbio_servo_get_speed_user(self->srv, pb_obj_get_positive_int(window_in), &speed));
    return mp_obj_new_int(speed);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_speed_obj, 1, common_Motor_speed);

// pybricks._common.Motor.run
STATIC mp_obj_t common_Motor_run(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed));

    mp_int_t speed = pb_obj_get_int(speed_in);
    pb_assert(pbio_servo_run_forever(self->srv, speed));
    pb_type_awaitable_update_all(self->awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_obj, 1, common_Motor_run);

// pybricks._common.Motor.hold
STATIC mp_obj_t common_Motor_hold(mp_obj_t self_in) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_servo_stop(self->srv, PBIO_CONTROL_ON_COMPLETION_HOLD));
    pb_type_awaitable_update_all(self->awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Motor_hold_obj, common_Motor_hold);

// pybricks._common.Motor.run_time
STATIC mp_obj_t common_Motor_run_time(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(time),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t time = pbio_int_math_max(pb_obj_get_int(time_in), 0);

    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Call pbio with parsed user/default arguments
    pb_assert(pbio_servo_run_time(self->srv, speed, time, then));

    // Old way to do parallel movement is to start and not wait on anything.
    if (!mp_obj_is_true(wait_in)) {
        return mp_const_none;
    }
    // Handle completion by awaiting or blocking.
    return await_or_wait(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_time_obj, 1, common_Motor_run_time);

STATIC mp_obj_t common_Motor_stall_return_value(mp_obj_t self_in) {

    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return the angle upon completion of the stall maneuver.
    int32_t stall_angle, stall_speed;
    pb_assert(pbio_servo_get_state_user(self->srv, &stall_angle, &stall_speed));

    return mp_obj_new_int(stall_angle);
}

// pybricks._common.Motor.run_until_stalled
STATIC mp_obj_t common_Motor_run_until_stalled(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_COAST_obj),
        PB_ARG_DEFAULT_NONE(duty_limit));

    mp_int_t speed = pb_obj_get_int(speed_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // REVISIT: Use torque limit. See https://github.com/pybricks/support/issues/1069.
    int32_t torque_limit;
    if (duty_limit_in != mp_const_none) {
        int32_t voltage_limit = pbio_battery_get_voltage_from_duty_pct(pb_obj_get_pct(duty_limit_in));
        torque_limit = pbio_observer_voltage_to_torque(self->srv->observer.model, voltage_limit);
    } else {
        torque_limit = self->srv->control.settings.actuation_max;
    }

    // Start moving.
    pb_assert(pbio_servo_run_until_stalled(self->srv, speed, torque_limit, then));

    // Handle completion by awaiting or blocking.
    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(self),
        self->awaitables,
        pb_type_awaitable_end_time_none,
        common_Motor_test_completion,
        common_Motor_stall_return_value,
        common_Motor_cancel,
        PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_until_stalled_obj, 1, common_Motor_run_until_stalled);

// pybricks._common.Motor.run_angle
STATIC mp_obj_t common_Motor_run_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(rotation_angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t angle = pb_obj_get_int(rotation_angle_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Call pbio with parsed user/default arguments
    pb_assert(pbio_servo_run_angle(self->srv, speed, angle, then));

    // Old way to do parallel movement is to start and not wait on anything.
    if (!mp_obj_is_true(wait_in)) {
        return mp_const_none;
    }
    // Handle completion by awaiting or blocking.
    return await_or_wait(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_angle_obj, 1, common_Motor_run_angle);

// pybricks._common.Motor.run_target
STATIC mp_obj_t common_Motor_run_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_REQUIRED(target_angle),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_HOLD_obj),
        PB_ARG_DEFAULT_TRUE(wait));

    mp_int_t speed = pb_obj_get_int(speed_in);
    mp_int_t target_angle = pb_obj_get_int(target_angle_in);
    pbio_control_on_completion_t then = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // Call pbio with parsed user/default arguments
    pb_assert(pbio_servo_run_target(self->srv, speed, target_angle, then));

    // Old way to do parallel movement is to start and not wait on anything.
    if (!mp_obj_is_true(wait_in)) {
        return mp_const_none;
    }
    // Handle completion by awaiting or blocking.
    return await_or_wait(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_target_obj, 1, common_Motor_run_target);

// pybricks._common.Motor.track_target
STATIC mp_obj_t common_Motor_track_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(target_angle));

    mp_int_t target_angle = pb_obj_get_int(target_angle_in);
    pb_assert(pbio_servo_track_target(self->srv, target_angle));
    pb_type_awaitable_update_all(self->awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_track_target_obj, 1, common_Motor_track_target);

// pybricks._common.Motor.stalled
STATIC mp_obj_t common_Motor_stalled(mp_obj_t self_in) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t stall_duration;
    bool stalled;
    pb_assert(pbio_servo_is_stalled(self->srv, &stalled, &stall_duration));
    return mp_obj_new_bool(stalled);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Motor_stalled_obj, common_Motor_stalled);

// pybricks._common.Motor.done
STATIC mp_obj_t common_Motor_done(mp_obj_t self_in) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(pbio_control_is_done(&self->srv->control));
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Motor_done_obj, common_Motor_done);

// pybricks._common.Motor.load
STATIC mp_obj_t common_Motor_load(mp_obj_t self_in) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t load;
    pb_assert(pbio_servo_get_load(self->srv, &load));
    return mp_obj_new_int(load);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Motor_load_obj, common_Motor_load);

#if PYBRICKS_PY_COMMON_CONTROL | PYBRICKS_PY_COMMON_LOGGER
STATIC const pb_attr_dict_entry_t common_Motor_attr_dict[] = {
    #if PYBRICKS_PY_COMMON_CONTROL
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_control, common_Motor_obj_t, control),
    #endif
    #if PYBRICKS_PY_COMMON_MOTOR_MODEL
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_model, common_Motor_obj_t, model),
    #endif
    #if PYBRICKS_PY_COMMON_LOGGER
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_log, common_Motor_obj_t, logger),
    #endif
    PB_ATTR_DICT_SENTINEL
};
#endif // PYBRICKS_PY_COMMON_CONTROL | PYBRICKS_PY_COMMON_LOGGER

// dir(pybricks.builtins.Motor)
STATIC const mp_rom_map_elem_t common_Motor_locals_dict_table[] = {
    //
    // Methods common to DC motors and encoded motors
    //
    { MP_ROM_QSTR(MP_QSTR_dc), MP_ROM_PTR(&common_DCMotor_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&common_DCMotor_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&common_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&common_DCMotor_dc_settings_obj) },
    //
    // Methods specific to encoded motors
    //
    { MP_ROM_QSTR(MP_QSTR_hold), MP_ROM_PTR(&common_Motor_hold_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&common_Motor_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_speed), MP_ROM_PTR(&common_Motor_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&common_Motor_reset_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&common_Motor_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_time), MP_ROM_PTR(&common_Motor_run_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_until_stalled), MP_ROM_PTR(&common_Motor_run_until_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_angle), MP_ROM_PTR(&common_Motor_run_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_target), MP_ROM_PTR(&common_Motor_run_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_stalled), MP_ROM_PTR(&common_Motor_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_done), MP_ROM_PTR(&common_Motor_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_track_target), MP_ROM_PTR(&common_Motor_track_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&common_Motor_load_obj) },
};
MP_DEFINE_CONST_DICT(common_Motor_locals_dict, common_Motor_locals_dict_table);

// type(pybricks.builtins.Motor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_Motor,
    MP_QSTR_Motor,
    MP_TYPE_FLAG_NONE,
    print, common_DCMotor_print,
    make_new, common_Motor_make_new,
    #if PYBRICKS_PY_COMMON_CONTROL | PYBRICKS_PY_COMMON_LOGGER
    attr, pb_attribute_handler,
    protocol, common_Motor_attr_dict,
    #endif
    locals_dict, &common_Motor_locals_dict);

#endif // PYBRICKS_PY_COMMON_MOTORS
