// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS

#include <math.h>
#include <stdlib.h>

#include <pbio/battery.h>
#include <pbio/int_math.h>
#include <pbio/servo.h>

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/robotics.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_awaitable.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>


#define PB_TYPE_CAR_MAX_DRIVE_MOTORS (PBIO_CONFIG_SERVO_NUM_DEV - 1)

// pybricks.robotics.Car class object
typedef struct {
    mp_obj_base_t base;
    pbio_servo_t *srv_steer;
    pbio_servo_t *srv_drive[PB_TYPE_CAR_MAX_DRIVE_MOTORS];
    size_t n_drive;
    int32_t max_angle;
} pb_type_Car_obj_t;

static int32_t run_until_stalled_blocking(pbio_servo_t *srv, pbio_direction_t direction, mp_obj_t torque_limit_in) {

    pb_module_tools_assert_blocking();

    // Use torque limit as percentage of max torque.
    mp_int_t torque_limit = srv->control.settings.actuation_max;
    if (torque_limit_in != mp_const_none) {
        torque_limit = torque_limit * pb_obj_get_pct(torque_limit_in) / 100;
    }

    // Begin the movement.
    int32_t speed = (direction == PBIO_DIRECTION_CLOCKWISE ? 1 : -1) * 300;
    pb_assert(pbio_servo_run_until_stalled(srv, speed, torque_limit, PBIO_CONTROL_ON_COMPLETION_COAST));

    uint32_t start_time = mp_hal_ticks_ms();

    // Wait for the movement to complete or be cancelled.
    while (!pbio_control_is_done(&srv->control)) {
        if (!pbio_servo_update_loop_is_running(srv)) {
            pb_assert(PBIO_ERROR_NO_DEV);
        }
        if (mp_hal_ticks_ms() - start_time > 10000) {
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("The steering mechanism has no end stop. Did you build a car yet?"));
        }
        mp_hal_delay_ms(10);
    }

    // Get the final state.
    int32_t stall_angle, stall_speed;
    pb_assert(pbio_servo_get_state_user(srv, &stall_angle, &stall_speed));
    return stall_angle;
}

// pybricks.robotics.Car.__init__
static mp_obj_t pb_type_Car_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(steer_motor),
        PB_ARG_REQUIRED(drive_motors),
        PB_ARG_DEFAULT_INT(torque_limit, 100));

    // Init can only be used in setup or blocking mode.
    pb_type_Car_obj_t *self = mp_obj_malloc(pb_type_Car_obj_t, type);

    // Should be one motor for steering.
    self->srv_steer = pb_type_motor_get_servo(steer_motor_in);

    if (pb_obj_is_array(drive_motors_in)) {
        // Unpack the drive motors if multiple are given.
        mp_obj_t *drive_motors;
        mp_obj_get_array(drive_motors_in, &self->n_drive, &drive_motors);
        if (self->n_drive > PB_TYPE_CAR_MAX_DRIVE_MOTORS || self->n_drive == 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("drive_motors must be a list of 1 to %d motors"), PB_TYPE_CAR_MAX_DRIVE_MOTORS);
        }
        for (size_t i = 0; i < self->n_drive; i++) {
            self->srv_drive[i] = pb_type_motor_get_servo(drive_motors[i]);
        }
    } else {
        // Otherwise use one motor.
        self->srv_drive[0] = pb_type_motor_get_servo(drive_motors_in);
        self->n_drive = 1;
    }

    for (size_t i = 0; i < self->n_drive; i++) {
        if (self->srv_drive[i] == self->srv_steer) {
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("drive_motors and steer_motor must be different"));
        }
        for (size_t j = i + 1; j < self->n_drive; j++) {
            if (self->srv_drive[i] == self->srv_drive[j]) {
                mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("drive_motors must be unique"));
            }
        }
    }

    // Find left and right end of steering range.
    int32_t angle_left = run_until_stalled_blocking(self->srv_steer, PBIO_DIRECTION_COUNTERCLOCKWISE, torque_limit_in);
    int32_t angle_right = run_until_stalled_blocking(self->srv_steer, PBIO_DIRECTION_CLOCKWISE, torque_limit_in);

    // Motor is now at the right end, so set angle accordingly.
    self->max_angle = (angle_right - angle_left) / 2;
    pb_assert(pbio_servo_reset_angle(self->srv_steer, self->max_angle, false));

    // Then steer to 0, which is now the center.
    pb_assert(pbio_servo_track_target(self->srv_steer, 0));
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.robotics.Car.drive_power
static mp_obj_t pb_type_Car_drive_power(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Car_obj_t, self,
        PB_ARG_REQUIRED(power));

    // Abstract unit of "power" that is useful for RC drive applications. This
    // is mostly linearly scaled duty cycle, with an offset for friction and
    // coasting at low power values to avoid braking.
    const int32_t power = pbio_int_math_clamp(pb_obj_get_int(power_in), 100);
    const bool do_coast = pbio_int_math_abs(power) < 10;
    const int32_t voltage = pbio_battery_get_voltage_from_duty_pct(power * 8 / 10 + 20 * pbio_int_math_sign(power));

    for (size_t i = 0; i < self->n_drive; i++) {
        pb_assert(pbio_dcmotor_user_command(self->srv_drive[i]->dcmotor, do_coast, voltage));
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Car_drive_power_obj, 1, pb_type_Car_drive_power);

// pybricks.robotics.Car.drive_speed
static mp_obj_t pb_type_Car_drive_speed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Car_obj_t, self,
        PB_ARG_REQUIRED(speed));

    const int32_t speed = pb_obj_get_int(speed_in);

    for (size_t i = 0; i < self->n_drive; i++) {
        pb_assert(pbio_servo_run_forever(self->srv_drive[i], speed));
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Car_drive_speed_obj, 1, pb_type_Car_drive_speed);

// pybricks.robotics.Car.steer
static mp_obj_t pb_type_Car_steer(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Car_obj_t, self,
        PB_ARG_REQUIRED(percentage));

    const int32_t percentage = pbio_int_math_clamp(pb_obj_get_int(percentage_in), 100);
    const int32_t angle = (self->max_angle - 10) * percentage / 100;
    pb_assert(pbio_servo_track_target(self->srv_steer, angle));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Car_steer_obj, 1, pb_type_Car_steer);

// dir(pybricks.robotics.Car)
static const mp_rom_map_elem_t pb_type_Car_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_drive_power),      MP_ROM_PTR(&pb_type_Car_drive_power_obj) },
    { MP_ROM_QSTR(MP_QSTR_drive_speed),      MP_ROM_PTR(&pb_type_Car_drive_speed_obj) },
    { MP_ROM_QSTR(MP_QSTR_steer),            MP_ROM_PTR(&pb_type_Car_steer_obj)       },
};
static MP_DEFINE_CONST_DICT(pb_type_Car_locals_dict, pb_type_Car_locals_dict_table);

// type(pybricks.robotics.Car)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_car,
    MP_QSTR_Car,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_Car_make_new,
    locals_dict, &pb_type_Car_locals_dict);

#endif // PYBRICKS_PY_ROBOTICS && PYBRICKS_PY_COMMON_MOTORS
