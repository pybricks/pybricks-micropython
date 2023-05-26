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

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

/**
 * A generator-like type for waiting on a motor operation to complete.
 */
typedef struct {
    mp_obj_base_t base;
    /**
     * Servo object that this object is awaiting on.
     */
    pbio_servo_t *srv;
} pb_type_MotorWait_obj_t;

// The __next__() method should raise a StopIteration if the operation is
// complete.
STATIC mp_obj_t pb_type_MotorWait_iternext(mp_obj_t self_in) {
    pb_type_MotorWait_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (0 /*cancelled*/) {
        // TODO: Handle cancelled.
        return MP_OBJ_STOP_ITERATION;
    }

    // Handle I/O exceptions.
    if (!pbio_servo_update_loop_is_running(self->srv)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }

    // If not done yet, keep going.
    if (!pbio_control_is_done(&self->srv->control)) {
        return mp_const_none;
    }

    // Otherwise we are completing a normal operation.
    return MP_OBJ_STOP_ITERATION;
}

// The close() method is used to cancel an operation before it completes. If
// the operation is already complete, it should do nothing. It can be called
// more than once.
STATIC mp_obj_t pb_type_MotorWait_close(mp_obj_t self_in) {
    pb_type_MotorWait_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // REVISIT: We only need to call an explicit stop if a task is cancelled
    // by something other than a motor.
    pb_assert(pbio_dcmotor_user_command(self->srv->dcmotor, true, 0));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_MotorWait_close_obj, pb_type_MotorWait_close);

STATIC const mp_rom_map_elem_t pb_type_MotorWait_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_MotorWait_close_obj) },
};
MP_DEFINE_CONST_DICT(pb_type_MotorWait_locals_dict, pb_type_MotorWait_locals_dict_table);

// This is a partial implementation of the Python generator type. It is missing
// send(value) and throw(type[, value[, traceback]])
MP_DEFINE_CONST_OBJ_TYPE(pb_type_MotorWait,
    MP_QSTR_MotorWait,
    MP_TYPE_FLAG_ITER_IS_ITERNEXT,
    iter, pb_type_MotorWait_iternext,
    locals_dict, &pb_type_MotorWait_locals_dict);

STATIC mp_obj_t pb_type_MotorWait_new(pbio_servo_t *srv) {
    pb_type_MotorWait_obj_t *self = m_new_obj(pb_type_MotorWait_obj_t);
    self->base.type = &pb_type_MotorWait;
    self->srv = srv;
    return MP_OBJ_FROM_PTR(self);
}

/* Wait for servo maneuver to complete */

STATIC void wait_for_completion(pbio_servo_t *srv) {
    while (!pbio_control_is_done(&srv->control)) {
        mp_hal_delay_ms(5);
    }
    if (!pbio_servo_update_loop_is_running(srv)) {
        pb_assert(PBIO_ERROR_NO_DEV);
    }
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

    return MP_OBJ_FROM_PTR(self);
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

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_obj, 1, common_Motor_run);

// pybricks._common.Motor.hold
STATIC mp_obj_t common_Motor_hold(mp_obj_t self_in) {
    common_Motor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_servo_stop(self->srv, PBIO_CONTROL_ON_COMPLETION_HOLD));
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

    // Handle async case, return a generator.
    if (pb_module_tools_run_loop_is_active()) {
        return pb_type_MotorWait_new(self->srv);
    }

    // Otherwise, handle default blocking wait.
    if (mp_obj_is_true(wait_in)) {
        wait_for_completion(self->srv);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_time_obj, 1, common_Motor_run_time);

// pybricks._common.Motor.run_until_stalled
STATIC mp_obj_t common_Motor_run_until_stalled(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(speed),
        PB_ARG_DEFAULT_OBJ(then, pb_Stop_COAST_obj),
        PB_ARG_DEFAULT_NONE(duty_limit));

    mp_int_t speed = pb_obj_get_int(speed_in);
    pbio_control_on_completion_t on_completion = pb_type_enum_get_value(then_in, &pb_enum_type_Stop);

    // If duty_limit argument given, limit duty during this maneuver.
    bool override_max_voltage = duty_limit_in != mp_const_none;
    int32_t max_voltage_old;

    if (override_max_voltage) {
        // Read original value so we can restore it when we're done
        pbio_dcmotor_get_settings(self->srv->dcmotor, &max_voltage_old);

        // Internally, the use of a duty cycle limit has been deprecated and
        // replaced by a voltage limit. Since we can't break the user API, we
        // convert the user duty limit (0--100) to a voltage by scaling it with
        // the battery voltage level, giving the same behavior as before.
        uint32_t max_voltage = pbio_battery_get_voltage_from_duty_pct(pb_obj_get_pct(duty_limit_in));

        // Apply the user limit
        pb_assert(pbio_dcmotor_set_settings(self->srv->dcmotor, max_voltage));
    }

    mp_obj_t ex = MP_OBJ_NULL;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // Start moving forever.
        pb_assert(pbio_servo_run_forever(self->srv, speed));

        // Wait until the motor stalls or stops on failure.
        uint32_t stall_duration;
        while (!pbio_control_is_stalled(&self->srv->control, &stall_duration)) {
            mp_hal_delay_ms(5);
        }

        // Assert that no errors happened in the update loop.
        if (!pbio_servo_update_loop_is_running(self->srv)) {
            pb_assert(PBIO_ERROR_NO_DEV);
        }

        nlr_pop();
    } else {
        ex = MP_OBJ_FROM_PTR(nlr.ret_val);
    }

    // Restore original settings
    if (override_max_voltage) {
        pb_assert(pbio_dcmotor_set_settings(self->srv->dcmotor, max_voltage_old));
    }

    if (ex != MP_OBJ_NULL) {
        nlr_raise(ex);
    }

    // Read the angle upon completion of the stall maneuver
    int32_t stall_angle, stall_speed;
    pb_assert(pbio_servo_get_state_user(self->srv, &stall_angle, &stall_speed));

    // Stop moving.
    pb_assert(pbio_servo_stop(self->srv, on_completion));

    // Return angle at which the motor stalled
    return mp_obj_new_int(stall_angle);
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

    // Handle async case, return a generator.
    if (pb_module_tools_run_loop_is_active()) {
        return pb_type_MotorWait_new(self->srv);
    }

    // Otherwise, handle default blocking wait.
    if (mp_obj_is_true(wait_in)) {
        wait_for_completion(self->srv);
    }

    return mp_const_none;
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

    // Handle async case, return a generator.
    if (pb_module_tools_run_loop_is_active()) {
        return pb_type_MotorWait_new(self->srv);
    }

    // Otherwise, handle default blocking wait.
    if (mp_obj_is_true(wait_in)) {
        wait_for_completion(self->srv);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Motor_run_target_obj, 1, common_Motor_run_target);

// pybricks._common.Motor.track_target
STATIC mp_obj_t common_Motor_track_target(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Motor_obj_t, self,
        PB_ARG_REQUIRED(target_angle));

    mp_int_t target_angle = pb_obj_get_int(target_angle_in);
    pb_assert(pbio_servo_track_target(self->srv, target_angle));

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
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&common_DCMotor_close_obj) },
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
