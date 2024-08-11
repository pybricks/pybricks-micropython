// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include <pbio/control.h>

#include "py/mpconfig.h"
#include "py/obj.h"

#include <pybricks/common.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

mp_obj_t make_acceleration_return_value(int32_t acceleration, int32_t deceleration) {
    // For backwards compatibility, return a single integer if acceleration
    // and deceleration are equal.
    if (acceleration == deceleration) {
        return mp_obj_new_int(acceleration);
    }
    // Otherwise return a tuple with both values.
    mp_obj_t accel[] = {
        mp_obj_new_int(acceleration),
        mp_obj_new_int(deceleration),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(accel), accel);
}

void unpack_acceleration_value(mp_obj_t accel_in, int32_t *acceleration, int32_t *deceleration) {
    // If no acceleration was given, leave values unchanged.
    if (accel_in == mp_const_none) {
        return;
    }

    // If single value is given for acceleration, use it for deceleration too.
    if (!pb_obj_is_array(accel_in)) {
        *acceleration = pb_obj_get_int(accel_in);
        *deceleration = *acceleration;
        return;
    }

    // Otherwise attempt to unpack acceleration and deceleration from tuple,
    // raising if something invalid was given.
    mp_obj_t *values;
    size_t n;
    mp_obj_get_array(accel_in, &n, &values);
    if (n != 2) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    *acceleration = pb_obj_get_int(values[0]);
    *deceleration = pb_obj_get_int(values[1]);
}

#if PYBRICKS_PY_COMMON_CONTROL

// pybricks._common.Control class object structure
typedef struct _pb_type_Control_obj_t {
    mp_obj_base_t base;
    pbio_control_t *control;
    mp_obj_t scale;
    #if PYBRICKS_PY_COMMON_LOGGER
    mp_obj_t logger;
    #endif
} pb_type_Control_obj_t;

// pybricks._common.Control.__init__/__new__
mp_obj_t pb_type_Control_obj_make_new(pbio_control_t *control) {

    pb_type_Control_obj_t *self = mp_obj_malloc(pb_type_Control_obj_t, &pb_type_Control);

    self->control = control;

    #if PYBRICKS_PY_COMMON_LOGGER
    // Create an instance of the Logger class
    self->logger = common_Logger_obj_make_new(&self->control->log, PBIO_CONTROL_LOGGER_NUM_COLS);
    #endif

    self->scale = mp_obj_new_int(control->settings.ctl_steps_per_app_step);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks._common.Control.limits
static mp_obj_t pb_type_Control_limits(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(speed),
        PB_ARG_DEFAULT_NONE(acceleration),
        PB_ARG_DEFAULT_NONE(torque));

    // Read current values.
    int32_t speed, acceleration, deceleration, torque;
    pbio_control_settings_get_trajectory_limits(&self->control->settings, &speed, &acceleration, &deceleration);
    torque = pbio_control_settings_get_actuation_limit(&self->control->settings);

    // If all given values are none, return current values
    if (speed_in == mp_const_none && acceleration_in == mp_const_none && torque_in == mp_const_none) {
        mp_obj_t ret[] = {
            mp_obj_new_int(speed),
            make_acceleration_return_value(acceleration, deceleration),
            mp_obj_new_int(torque),
        };
        return mp_obj_new_tuple(MP_ARRAY_SIZE(ret), ret);
    }

    // Set user settings if given, else keep using current values.
    speed = pb_obj_get_default_abs_int(speed_in, speed);
    torque = pb_obj_get_default_abs_int(torque_in, torque);
    unpack_acceleration_value(acceleration_in, &acceleration, &deceleration);

    // Set new values.
    pb_assert(pbio_control_settings_set_trajectory_limits(&self->control->settings, speed, acceleration, deceleration));
    pb_assert(pbio_control_settings_set_actuation_limit(&self->control->settings, torque));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Control_limits_obj, 1, pb_type_Control_limits);

// pybricks._common.Control.pid
static mp_obj_t pb_type_Control_pid(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(kp),
        PB_ARG_DEFAULT_NONE(ki),
        PB_ARG_DEFAULT_NONE(kd),
        PB_ARG_DEFAULT_NONE(integral_deadzone),
        PB_ARG_DEFAULT_NONE(integral_rate));

    // Read current values
    int32_t kp, ki, kd;
    int32_t integral_change_max, integral_deadzone;
    pbio_control_settings_get_pid(&self->control->settings, &kp, &ki, &kd, &integral_deadzone, &integral_change_max);

    // If all given values are none, return current values
    if (kp_in == mp_const_none && ki_in == mp_const_none && kd_in == mp_const_none &&
        integral_rate_in == mp_const_none) {
        mp_obj_t ret[5];
        ret[0] = mp_obj_new_int(kp);
        ret[1] = mp_obj_new_int(ki);
        ret[2] = mp_obj_new_int(kd);
        ret[3] = mp_obj_new_int(integral_deadzone);
        ret[4] = mp_obj_new_int(integral_change_max);
        return mp_obj_new_tuple(5, ret);
    }

    // Set user settings
    kp = pb_obj_get_default_abs_int(kp_in, kp);
    ki = pb_obj_get_default_abs_int(ki_in, ki);
    kd = pb_obj_get_default_abs_int(kd_in, kd);
    integral_change_max = pb_obj_get_default_abs_int(integral_rate_in, integral_change_max);
    integral_deadzone = pb_obj_get_default_abs_int(integral_deadzone_in, integral_deadzone);

    pb_assert(pbio_control_settings_set_pid(&self->control->settings, kp, ki, kd, integral_deadzone, integral_change_max));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Control_pid_obj, 1, pb_type_Control_pid);

// pybricks._common.Control.target_tolerances
static mp_obj_t pb_type_Control_target_tolerances(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(speed),
        PB_ARG_DEFAULT_NONE(position));

    // Read current values
    int32_t speed, position;
    pbio_control_settings_get_target_tolerances(&self->control->settings, &speed, &position);

    // If all given values are none, return current values
    if (speed_in == mp_const_none && position_in == mp_const_none) {
        mp_obj_t ret[2];
        ret[0] = mp_obj_new_int(speed);
        ret[1] = mp_obj_new_int(position);
        return mp_obj_new_tuple(2, ret);
    }

    // Set user settings
    speed = pb_obj_get_default_abs_int(speed_in, speed);
    position = pb_obj_get_default_abs_int(position_in, position);

    pb_assert(pbio_control_settings_set_target_tolerances(&self->control->settings, speed, position));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Control_target_tolerances_obj, 1, pb_type_Control_target_tolerances);

// pybricks._common.Control.stall_tolerances
static mp_obj_t pb_type_Control_stall_tolerances(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(speed),
        PB_ARG_DEFAULT_NONE(time));

    // Read current values
    int32_t speed;
    uint32_t time;
    pbio_control_settings_get_stall_tolerances(&self->control->settings, &speed, &time);

    // If all given values are none, return current values
    if (speed_in == mp_const_none && time_in == mp_const_none) {
        mp_obj_t ret[2];
        ret[0] = mp_obj_new_int(speed);
        ret[1] = mp_obj_new_int_from_uint(time);
        return mp_obj_new_tuple(2, ret);
    }

    // Set user settings
    speed = pb_obj_get_default_abs_int(speed_in, speed);
    time = pb_obj_get_default_abs_int(time_in, time);

    pb_assert(pbio_control_settings_set_stall_tolerances(&self->control->settings, speed, time));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Control_stall_tolerances_obj, 1, pb_type_Control_stall_tolerances);

// pybricks._common.Control.trajectory
static mp_obj_t pb_type_Control_trajectory(mp_obj_t self_in) {
    pb_type_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_trajectory_t *trj = &self->control->trajectory;

    mp_obj_t parms[13];

    if (pbio_control_is_active(self->control)) {
        parms[0] = mp_obj_new_int(0);
        parms[1] = mp_obj_new_int(trj->t1 / 10);
        parms[2] = mp_obj_new_int(trj->t2 / 10);
        parms[3] = mp_obj_new_int(trj->t3 / 10);
        parms[4] = mp_obj_new_int(0);
        parms[5] = mp_obj_new_int(trj->th1 / 1000);
        parms[6] = mp_obj_new_int(trj->th2 / 1000);
        parms[7] = mp_obj_new_int(trj->th3 / 1000);
        parms[8] = mp_obj_new_int(trj->w0 / 10);
        parms[9] = mp_obj_new_int(trj->w1 / 10);
        parms[10] = mp_obj_new_int(trj->w3 / 10);
        parms[11] = mp_obj_new_int(trj->a0);
        parms[12] = mp_obj_new_int(trj->a2);
        return mp_obj_new_tuple(13, parms);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Control_trajectory_obj, pb_type_Control_trajectory);

// DELETEME: This method should not be used in V3.2 or later. It may be removed
// in a future version
// pybricks._common.Control.done
static mp_obj_t pb_type_Control_done(mp_obj_t self_in) {
    pb_type_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(pbio_control_is_done(self->control));
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Control_done_obj, pb_type_Control_done);

// DELETEME: This method should not be used in V3.2 or later. It may be removed
// in a future version
// pybricks._common.Control.load
static mp_obj_t pb_type_Control_load(mp_obj_t self_in) {
    pb_type_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!pbio_control_is_active(self->control)) {
        return mp_obj_new_int(0);
    }

    // Read currently applied PID feedback torque and return as mNm.
    return mp_obj_new_int(self->control->pid_average / 1000);
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Control_load_obj, pb_type_Control_load);

// DELETEME: This method should not be used in V3.2 or later. It may be removed
// in a future version
// pybricks._common.Control.stalled
static mp_obj_t pb_type_Control_stalled(mp_obj_t self_in) {
    pb_type_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t stall_duration;
    return mp_obj_new_bool(pbio_control_is_stalled(self->control, &stall_duration));
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Control_stalled_obj, pb_type_Control_stalled);

static const pb_attr_dict_entry_t pb_type_Control_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_scale, pb_type_Control_obj_t, scale),
    #if PYBRICKS_PY_COMMON_LOGGER
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_log, pb_type_Control_obj_t, logger),
    #endif // PYBRICKS_PY_COMMON_LOGGER
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.common.Control)
static const mp_rom_map_elem_t pb_type_Control_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_limits), MP_ROM_PTR(&pb_type_Control_limits_obj) },
    { MP_ROM_QSTR(MP_QSTR_pid), MP_ROM_PTR(&pb_type_Control_pid_obj) },
    { MP_ROM_QSTR(MP_QSTR_target_tolerances), MP_ROM_PTR(&pb_type_Control_target_tolerances_obj) },
    { MP_ROM_QSTR(MP_QSTR_stall_tolerances), MP_ROM_PTR(&pb_type_Control_stall_tolerances_obj) },
    { MP_ROM_QSTR(MP_QSTR_trajectory), MP_ROM_PTR(&pb_type_Control_trajectory_obj) },
    { MP_ROM_QSTR(MP_QSTR_done), MP_ROM_PTR(&pb_type_Control_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&pb_type_Control_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_stalled), MP_ROM_PTR(&pb_type_Control_stalled_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_Control_locals_dict, pb_type_Control_locals_dict_table);

// type(pybricks.common.Control)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_Control,
    MP_QSTR_Control,
    MP_TYPE_FLAG_NONE,
    attr, pb_attribute_handler,
    protocol, pb_type_Control_attr_dict,
    locals_dict, &pb_type_Control_locals_dict);

#endif // PYBRICKS_PY_COMMON_CONTROL
