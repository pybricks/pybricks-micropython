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
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

pbio_servo_t *pb_type_motor_get_servo(mp_obj_t motor_in) {
    return ((pb_type_Motor_obj_t *)pb_obj_get_base_class_obj(motor_in, &pb_type_Motor))->srv;
}

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

// pybricks.common.Motor.__init__
static mp_obj_t pb_type_Motor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_OBJ(positive_direction, pb_Direction_CLOCKWISE_obj),
        PB_ARG_DEFAULT_NONE(gears),
        PB_ARG_DEFAULT_TRUE(reset_angle),
        PB_ARG_DEFAULT_NONE(profile));

    // Validate arguments before attempting to set up the motor.
    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_direction_t positive_direction = pb_type_enum_get_value(positive_direction_in, &pb_enum_type_Direction);

    pb_type_Motor_obj_t *self = mp_obj_malloc(pb_type_Motor_obj_t, type);
    self->port = port;

    // Initialize device and get resulting device ID.
    pbdrv_legodev_type_id_t type_id = pb_type_device_init_class(&self->device_base, port_in,
        type == &pb_type_DCMotor ? PBDRV_LEGODEV_TYPE_ID_ANY_DC_MOTOR : PBDRV_LEGODEV_TYPE_ID_ANY_ENCODED_MOTOR);

    pb_assert(pbio_servo_get_servo(self->device_base.legodev, &self->srv));

    // For a DC motor, all we need to do is get the dc motor device.
    if (type == &pb_type_DCMotor) {
        // Parse args again but this time restrict to 2 args, in order to raise
        // the appropriate exception if non-DCMotor arguments are given.
        mp_arg_parse_all(n_args, args, &kw_args, 2, allowed_args, parsed_args);
        pb_assert(pbio_dcmotor_setup(self->srv->dcmotor, type_id, positive_direction));
        return MP_OBJ_FROM_PTR(self);
    }

    bool reset_angle = mp_obj_is_true(reset_angle_in);
    int32_t precision_profile = pb_obj_get_default_abs_int(profile_in, 0);

    // Set up servo
    int32_t gear_ratio = get_gear_ratio(gears_in);
    pb_assert(pbio_servo_setup(self->srv, type_id, positive_direction, gear_ratio, reset_angle, precision_profile));

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

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.common.Motor.__repr__
static void pb_type_Motor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "%q(Port.%c, %q.%q)",
        self->device_base.base.type->name, self->port, MP_QSTR_Direction,
        self->srv->dcmotor->direction == PBIO_DIRECTION_CLOCKWISE ? MP_QSTR_CLOCKWISE : MP_QSTR_COUNTERCLOCKWISE);
}

// pybricks.common.Motor.dc
static mp_obj_t pb_type_Motor_duty(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
        PB_ARG_REQUIRED(duty));

    // pbio has only voltage setters now, but the .dc() method will continue to
    // exist for backwards compatibility. So, we convert duty cycle to voltages.
    int32_t voltage = pbio_battery_get_voltage_from_duty_pct(pb_obj_get_int(duty_in));
    pb_assert(pbio_dcmotor_user_command(self->srv->dcmotor, false, voltage));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_duty_obj, 1, pb_type_Motor_duty);

// pybricks.common.Motor.stop
static mp_obj_t pb_type_Motor_stop(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_dcmotor_user_command(self->srv->dcmotor, true, 0));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_stop_obj, pb_type_Motor_stop);

// pybricks.common.Motor.brake
static mp_obj_t pb_type_Motor_brake(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_dcmotor_user_command(self->srv->dcmotor, false, 0));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_brake_obj, pb_type_Motor_brake);

// pybricks.common.Motor.dc
static mp_obj_t pb_type_Motor_dc_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
        PB_ARG_DEFAULT_NONE(max_voltage));

    // If no arguments given, return existing values
    if (max_voltage_in == mp_const_none) {
        int32_t max_voltage_now;
        pbio_dcmotor_get_settings(self->srv->dcmotor, &max_voltage_now);
        mp_obj_t retval[1];
        retval[0] = mp_obj_new_int(max_voltage_now);
        return mp_obj_new_tuple(1, retval);
    }

    // Set the new limit
    pb_assert(pbio_dcmotor_set_settings(self->srv->dcmotor, pb_obj_get_int(max_voltage_in)));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_settings_obj, 1, pb_type_Motor_dc_settings);

// pybricks.common.Motor.close
static mp_obj_t pb_type_Motor_close(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_dcmotor_close(self->srv->dcmotor));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_close_obj, pb_type_Motor_close);

// pybricks.common.Motor.angle
static mp_obj_t pb_type_Motor_angle(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t angle, speed;
    pb_assert(pbio_servo_get_state_user(self->srv, &angle, &speed));
    return mp_obj_new_int(angle);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_angle_obj, pb_type_Motor_angle);

// pybricks.common.Motor.reset_angle
static mp_obj_t pb_type_Motor_reset_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
        PB_ARG_DEFAULT_NONE(angle));

    // If no angle argument is given, reset to the absolute value
    bool reset_to_abs = angle_in == mp_const_none;

    // Otherwise reset to the given angle
    mp_int_t reset_angle = reset_to_abs ? 0 : pb_obj_get_int(angle_in);

    // Set the new angle
    pb_assert(pbio_servo_reset_angle(self->srv, reset_angle, reset_to_abs));
    pb_type_awaitable_update_all(self->device_base.awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_reset_angle_obj, 1, pb_type_Motor_reset_angle);

// pybricks.common.Motor.speed
static mp_obj_t pb_type_Motor_speed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
        PB_ARG_DEFAULT_INT(window, 100));

    int32_t speed;
    pb_assert(pbio_servo_get_speed_user(self->srv, pb_obj_get_positive_int(window_in), &speed));
    return mp_obj_new_int(speed);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_speed_obj, 1, pb_type_Motor_speed);

// pybricks.common.Motor.run
static mp_obj_t pb_type_Motor_run(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed));

    mp_int_t speed = pb_obj_get_int(speed_in);
    pb_assert(pbio_servo_run_forever(self->srv, speed));
    pb_type_awaitable_update_all(self->device_base.awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_run_obj, 1, pb_type_Motor_run);

// pybricks.common.Motor.hold
static mp_obj_t pb_type_Motor_hold(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_servo_stop(self->srv, PBIO_CONTROL_ON_COMPLETION_HOLD));
    pb_type_awaitable_update_all(self->device_base.awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_hold_obj, pb_type_Motor_hold);

static bool pb_type_Motor_test_completion(mp_obj_t self_in, uint32_t end_time) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Handle I/O exceptions like port unplugged.
    if (!pbio_servo_update_loop_is_running(self->srv)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // Get completion state.
    return pbio_control_is_done(&self->srv->control);
}

static void pb_type_Motor_cancel(mp_obj_t self_in) {
    pb_type_Motor_stop(self_in);
}

// Common awaitable used for most motor methods.
static mp_obj_t await_or_wait(pb_type_Motor_obj_t *self) {
    return pb_type_awaitable_await_or_wait(
        MP_OBJ_FROM_PTR(self),
        self->device_base.awaitables,
        pb_type_awaitable_end_time_none,
        pb_type_Motor_test_completion,
        pb_type_awaitable_return_none,
        pb_type_Motor_cancel,
        PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
}

// pybricks.common.Motor.run_time
static mp_obj_t pb_type_Motor_run_time(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
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
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_run_time_obj, 1, pb_type_Motor_run_time);

static mp_obj_t pb_type_Motor_stall_return_value(mp_obj_t self_in) {

    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return the angle upon completion of the stall maneuver.
    int32_t stall_angle, stall_speed;
    pb_assert(pbio_servo_get_state_user(self->srv, &stall_angle, &stall_speed));

    return mp_obj_new_int(stall_angle);
}

// pybricks.common.Motor.run_until_stalled
static mp_obj_t pb_type_Motor_run_until_stalled(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
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
        self->device_base.awaitables,
        pb_type_awaitable_end_time_none,
        pb_type_Motor_test_completion,
        pb_type_Motor_stall_return_value,
        pb_type_Motor_cancel,
        PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_run_until_stalled_obj, 1, pb_type_Motor_run_until_stalled);

// pybricks.common.Motor.run_angle
static mp_obj_t pb_type_Motor_run_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
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
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_run_angle_obj, 1, pb_type_Motor_run_angle);

// pybricks.common.Motor.run_target
static mp_obj_t pb_type_Motor_run_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
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
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_run_target_obj, 1, pb_type_Motor_run_target);

// pybricks.common.Motor.track_target
static mp_obj_t pb_type_Motor_track_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Motor_obj_t, self,
        PB_ARG_REQUIRED(target_angle));

    mp_int_t target_angle = pb_obj_get_int(target_angle_in);
    pb_assert(pbio_servo_track_target(self->srv, target_angle));
    pb_type_awaitable_update_all(self->device_base.awaitables, PB_TYPE_AWAITABLE_OPT_CANCEL_ALL);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Motor_track_target_obj, 1, pb_type_Motor_track_target);

// pybricks.common.Motor.stalled
static mp_obj_t pb_type_Motor_stalled(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t stall_duration;
    bool stalled;
    pb_assert(pbio_servo_is_stalled(self->srv, &stalled, &stall_duration));
    return mp_obj_new_bool(stalled);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_stalled_obj, pb_type_Motor_stalled);

// pybricks.common.Motor.done
static mp_obj_t pb_type_Motor_done(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(pbio_control_is_done(&self->srv->control));
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_done_obj, pb_type_Motor_done);

// pybricks.common.Motor.load
static mp_obj_t pb_type_Motor_load(mp_obj_t self_in) {
    pb_type_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t load;
    pb_assert(pbio_servo_get_load(self->srv, &load));
    return mp_obj_new_int(load);
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Motor_load_obj, pb_type_Motor_load);

#if PYBRICKS_PY_COMMON_CONTROL | PYBRICKS_PY_COMMON_LOGGER
static const pb_attr_dict_entry_t pb_type_Motor_attr_dict[] = {
    #if PYBRICKS_PY_COMMON_CONTROL
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_control, pb_type_Motor_obj_t, control),
    #endif
    #if PYBRICKS_PY_COMMON_MOTOR_MODEL
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_model, pb_type_Motor_obj_t, model),
    #endif
    #if PYBRICKS_PY_COMMON_LOGGER
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_log, pb_type_Motor_obj_t, logger),
    #endif
    PB_ATTR_DICT_SENTINEL
};
#endif // PYBRICKS_PY_COMMON_CONTROL | PYBRICKS_PY_COMMON_LOGGER

// dir(pybricks.builtins.Motor)
static const mp_rom_map_elem_t pb_type_Motor_locals_dict_table[] = {
    //
    // Methods common to DC motors and encoded motors
    //
    { MP_ROM_QSTR(MP_QSTR_dc), MP_ROM_PTR(&pb_type_Motor_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&pb_type_Motor_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&pb_type_Motor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&pb_type_Motor_settings_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_Motor_close_obj) },
    //
    // Methods specific to encoded motors
    //
    { MP_ROM_QSTR(MP_QSTR_hold), MP_ROM_PTR(&pb_type_Motor_hold_obj) },
    { MP_ROM_QSTR(MP_QSTR_angle), MP_ROM_PTR(&pb_type_Motor_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_speed), MP_ROM_PTR(&pb_type_Motor_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&pb_type_Motor_reset_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&pb_type_Motor_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_time), MP_ROM_PTR(&pb_type_Motor_run_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_until_stalled), MP_ROM_PTR(&pb_type_Motor_run_until_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_angle), MP_ROM_PTR(&pb_type_Motor_run_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_target), MP_ROM_PTR(&pb_type_Motor_run_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_stalled), MP_ROM_PTR(&pb_type_Motor_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_done), MP_ROM_PTR(&pb_type_Motor_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_track_target), MP_ROM_PTR(&pb_type_Motor_track_target_obj) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&pb_type_Motor_load_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_Motor_locals_dict, pb_type_Motor_locals_dict_table);

// The DC Motor methods are shared with the DCMotor class, so use the first section of the same table.
static MP_DEFINE_CONST_DICT_WITH_SIZE(pb_type_DCMotor_locals_dict, pb_type_Motor_locals_dict_table, 5);

// type(pybricks.builtins.DCMotor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_DCMotor,
    MP_QSTR_DCMotor,
    MP_TYPE_FLAG_NONE,
    print, pb_type_Motor_print,
    make_new, pb_type_Motor_make_new,
    locals_dict, &pb_type_DCMotor_locals_dict);

// type(pybricks.builtins.Motor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_Motor,
    MP_QSTR_Motor,
    MP_TYPE_FLAG_NONE,
    print, pb_type_Motor_print,
    make_new, pb_type_Motor_make_new,
    #if PYBRICKS_PY_COMMON_CONTROL | PYBRICKS_PY_COMMON_LOGGER
    attr, pb_attribute_handler,
    protocol, pb_type_Motor_attr_dict,
    #endif
    locals_dict, &pb_type_Motor_locals_dict);

#endif // PYBRICKS_PY_COMMON_MOTORS
