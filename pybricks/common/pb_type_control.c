// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_MOTORS

#include <pbio/control.h>

#include "py/obj.h"

#include <pybricks/common.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

// pybricks._common.Control class object structure
typedef struct _common_Control_obj_t {
    mp_obj_base_t base;
    pbio_control_t *control;
    mp_obj_t scale;
    #if PYBRICKS_PY_COMMON_LOGGER
    mp_obj_t logger;
    #endif
} common_Control_obj_t;

// pybricks._common.Control.__init__/__new__
mp_obj_t common_Control_obj_make_new(pbio_control_t *control) {

    common_Control_obj_t *self = m_new_obj(common_Control_obj_t);
    self->base.type = &pb_type_Control;

    self->control = control;

    #if PYBRICKS_PY_COMMON_LOGGER
    // Create an instance of the Logger class
    self->logger = common_Logger_obj_make_new(&self->control->log, PBIO_CONTROL_LOG_COLS);
    #endif

    #if MICROPY_PY_BUILTINS_FLOAT
    self->scale = mp_obj_new_float(fix16_to_float(control->settings.counts_per_unit));
    #else
    self->scale = mp_obj_new_int(fix16_to_int(control->settings.counts_per_unit));
    #endif

    return self;
}

// pybricks._common.Control.limits
STATIC mp_obj_t common_Control_limits(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(speed),
        PB_ARG_DEFAULT_NONE(acceleration),
        PB_ARG_DEFAULT_NONE(torque));

    // Read current values
    int32_t speed, acceleration, torque;
    pbio_control_settings_get_limits(&self->control->settings, &speed, &acceleration, &torque);

    // If all given values are none, return current values
    if (speed_in == mp_const_none && acceleration_in == mp_const_none && torque_in == mp_const_none) {
        mp_obj_t ret[3];
        ret[0] = mp_obj_new_int(speed);
        ret[1] = mp_obj_new_int(acceleration);
        ret[2] = mp_obj_new_int(torque);
        return mp_obj_new_tuple(3, ret);
    }

    // Set user settings
    speed = pb_obj_get_default_int(speed_in, speed);
    acceleration = pb_obj_get_default_int(acceleration_in, acceleration);
    torque = pb_obj_get_default_int(torque_in, torque);

    pb_assert(pbio_control_settings_set_limits(&self->control->settings, speed, acceleration, torque));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Control_limits_obj, 1, common_Control_limits);

// pybricks._common.Control.pid
STATIC mp_obj_t common_Control_pid(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(kp),
        PB_ARG_DEFAULT_NONE(ki),
        PB_ARG_DEFAULT_NONE(kd),
        PB_ARG_DEFAULT_NONE(reserved),
        PB_ARG_DEFAULT_NONE(integral_rate));

    // Read current values
    int32_t kp, ki, kd;
    int32_t integral_rate;
    pbio_control_settings_get_pid(&self->control->settings, &kp, &ki, &kd, &integral_rate);

    // If all given values are none, return current values
    (void)reserved_in;
    if (kp_in == mp_const_none && ki_in == mp_const_none && kd_in == mp_const_none &&
        integral_rate_in == mp_const_none) {
        mp_obj_t ret[5];
        ret[0] = mp_obj_new_int(kp);
        ret[1] = mp_obj_new_int(ki);
        ret[2] = mp_obj_new_int(kd);
        ret[3] = mp_const_none;
        ret[4] = mp_obj_new_int(integral_rate);
        return mp_obj_new_tuple(5, ret);
    }

    // Set user settings
    kp = pb_obj_get_default_int(kp_in, kp);
    ki = pb_obj_get_default_int(ki_in, ki);
    kd = pb_obj_get_default_int(kd_in, kd);
    integral_rate = pb_obj_get_default_int(integral_rate_in, integral_rate);

    pb_assert(pbio_control_settings_set_pid(&self->control->settings, kp, ki, kd, integral_rate));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Control_pid_obj, 1, common_Control_pid);

// pybricks._common.Control.target_tolerances
STATIC mp_obj_t common_Control_target_tolerances(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Control_obj_t, self,
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
    speed = pb_obj_get_default_int(speed_in, speed);
    position = pb_obj_get_default_int(position_in, position);

    pb_assert(pbio_control_settings_set_target_tolerances(&self->control->settings, speed, position));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Control_target_tolerances_obj, 1, common_Control_target_tolerances);

// pybricks._common.Control.stall_tolerances
STATIC mp_obj_t common_Control_stall_tolerances(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(speed),
        PB_ARG_DEFAULT_NONE(time));

    // Read current values
    int32_t speed, time;
    pbio_control_settings_get_stall_tolerances(&self->control->settings, &speed, &time);

    // If all given values are none, return current values
    if (speed_in == mp_const_none && time_in == mp_const_none) {
        mp_obj_t ret[2];
        ret[0] = mp_obj_new_int(speed);
        ret[1] = mp_obj_new_int(time);
        return mp_obj_new_tuple(2, ret);
    }

    // Set user settings
    speed = pb_obj_get_default_int(speed_in, speed);
    time = pb_obj_get_default_int(time_in, time);

    pb_assert(pbio_control_settings_set_stall_tolerances(&self->control->settings, speed, time));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Control_stall_tolerances_obj, 1, common_Control_stall_tolerances);

// pybricks._common.Control.trajectory
STATIC mp_obj_t common_Control_trajectory(mp_obj_t self_in) {
    common_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_trajectory_t trajectory;

    mp_obj_t parms[12];

    trajectory = self->control->trajectory;

    if (pbio_control_is_active(self->control)) {
        parms[0] = mp_obj_new_int((trajectory.t0 - trajectory.t0) / 1000);
        parms[1] = mp_obj_new_int((trajectory.t1 - trajectory.t0) / 1000);
        parms[2] = mp_obj_new_int((trajectory.t2 - trajectory.t0) / 1000);
        parms[3] = mp_obj_new_int((trajectory.t3 - trajectory.t0) / 1000);
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
MP_DEFINE_CONST_FUN_OBJ_1(common_Control_trajectory_obj, common_Control_trajectory);

// pybricks._common.Control.done
STATIC mp_obj_t common_Control_done(mp_obj_t self_in) {
    common_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(pbio_control_is_done(self->control));
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Control_done_obj, common_Control_done);

// pybricks._common.Control.load
STATIC mp_obj_t common_Control_load(mp_obj_t self_in) {
    common_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Read currently applied PID feedback torque and return as mNm.
    return mp_obj_new_int(pbio_control_get_load(self->control) / 1000);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Control_load_obj, common_Control_load);

// pybricks._common.Control.stalled
STATIC mp_obj_t common_Control_stalled(mp_obj_t self_in) {
    common_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(pbio_control_is_stalled(self->control));
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Control_stalled_obj, common_Control_stalled);

// dir(pybricks.common.Control)
STATIC const mp_rom_map_elem_t common_Control_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_limits), MP_ROM_PTR(&common_Control_limits_obj) },
    { MP_ROM_QSTR(MP_QSTR_pid), MP_ROM_PTR(&common_Control_pid_obj) },
    { MP_ROM_QSTR(MP_QSTR_target_tolerances), MP_ROM_PTR(&common_Control_target_tolerances_obj) },
    { MP_ROM_QSTR(MP_QSTR_stall_tolerances), MP_ROM_PTR(&common_Control_stall_tolerances_obj) },
    { MP_ROM_QSTR(MP_QSTR_trajectory), MP_ROM_PTR(&common_Control_trajectory_obj) },
    { MP_ROM_QSTR(MP_QSTR_done), MP_ROM_PTR(&common_Control_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&common_Control_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_stalled), MP_ROM_PTR(&common_Control_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_ATTRIBUTE_OFFSET(common_Control_obj_t, scale) },
    #if PYBRICKS_PY_COMMON_LOGGER
    { MP_ROM_QSTR(MP_QSTR_log), MP_ROM_ATTRIBUTE_OFFSET(common_Control_obj_t, logger) },
    #endif // PYBRICKS_PY_COMMON_LOGGER
};
STATIC MP_DEFINE_CONST_DICT(common_Control_locals_dict, common_Control_locals_dict_table);

// type(pybricks.common.Control)
const mp_obj_type_t pb_type_Control = {
    { &mp_type_type },
    .name = MP_QSTR_Control,
    .locals_dict = (mp_obj_dict_t *)&common_Control_locals_dict,
};

#endif // PYBRICKS_PY_COMMON_MOTORS
