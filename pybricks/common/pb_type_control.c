// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_MOTORS

#include "common/common_motors.h"

#include <pbio/control.h>

#include "py/obj.h"

#include "util_pb/pb_error.h"
#include "util_mp/pb_obj_helper.h"
#include "util_mp/pb_kwarg_helper.h"

// pybricks._common.Control class object structure
typedef struct _common_Control_obj_t {
    mp_obj_base_t base;
    pbio_control_t *control;
    mp_obj_t scale;
} common_Control_obj_t;

// pybricks._common.Control.__init__/__new__
mp_obj_t common_Control_obj_make_new(pbio_control_t *control) {

    common_Control_obj_t *self = m_new_obj(common_Control_obj_t);
    self->base.type = &pb_type_Control;

    self->control = control;

    #if MICROPY_PY_BUILTINS_FLOAT
    self->scale = mp_obj_new_float(fix16_to_float(control->settings.counts_per_unit));
    #else
    self->scale = mp_obj_new_int(fix16_to_int(control->settings.counts_per_unit));
    #endif

    return self;
}

STATIC void raise_if_control_busy(pbio_control_t *ctl) {
    if (ctl->type != PBIO_CONTROL_NONE) {
        pb_assert(PBIO_ERROR_INVALID_OP);
    }
}

// pybricks._common.Control.limits
STATIC mp_obj_t common_Control_limits(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(speed),
        PB_ARG_DEFAULT_NONE(acceleration),
        PB_ARG_DEFAULT_NONE(actuation));

    // Read current values
    int32_t _speed, _acceleration, _actuation;
    pbio_control_settings_get_limits(&self->control->settings, &_speed, &_acceleration, &_actuation);

    // If all given values are none, return current values
    if (speed == mp_const_none && acceleration == mp_const_none && actuation == mp_const_none) {
        mp_obj_t ret[3];
        ret[0] = mp_obj_new_int(_speed);
        ret[1] = mp_obj_new_int(_acceleration);
        ret[2] = mp_obj_new_int(_actuation);
        return mp_obj_new_tuple(3, ret);
    }

    // Assert control is not active
    raise_if_control_busy(self->control);

    // Set user settings
    _speed = pb_obj_get_default_int(speed, _speed);
    _acceleration = pb_obj_get_default_int(acceleration, _acceleration);
    _actuation = pb_obj_get_default_int(actuation, _actuation);

    pb_assert(pbio_control_settings_set_limits(&self->control->settings, _speed, _acceleration, _actuation));

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
        PB_ARG_DEFAULT_NONE(integral_range),
        PB_ARG_DEFAULT_NONE(integral_rate),
        PB_ARG_DEFAULT_NONE(feed_forward));

    // Read current values
    int16_t _kp, _ki, _kd;
    int32_t _integral_range, _integral_rate, _feed_forward;
    pbio_control_settings_get_pid(&self->control->settings, &_kp, &_ki, &_kd, &_integral_range, &_integral_rate, &_feed_forward);

    // If all given values are none, return current values
    if (kp == mp_const_none && ki == mp_const_none && kd == mp_const_none &&
        integral_range == mp_const_none && integral_rate == mp_const_none && feed_forward == mp_const_none) {
        mp_obj_t ret[6];
        ret[0] = mp_obj_new_int(_kp);
        ret[1] = mp_obj_new_int(_ki);
        ret[2] = mp_obj_new_int(_kd);
        ret[3] = mp_obj_new_int(_integral_range);
        ret[4] = mp_obj_new_int(_integral_rate);
        ret[5] = mp_obj_new_int(_feed_forward);
        return mp_obj_new_tuple(6, ret);
    }

    // Assert control is not active
    raise_if_control_busy(self->control);

    // Set user settings
    _kp = pb_obj_get_default_int(kp, _kp);
    _ki = pb_obj_get_default_int(ki, _ki);
    _kd = pb_obj_get_default_int(kd, _kd);
    _integral_range = pb_obj_get_default_int(integral_range, _integral_range);
    _integral_rate = pb_obj_get_default_int(integral_rate, _integral_rate);
    _feed_forward = pb_obj_get_default_int(feed_forward, _feed_forward);

    pb_assert(pbio_control_settings_set_pid(&self->control->settings, _kp, _ki, _kd, _integral_range, _integral_rate, _feed_forward));

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
    int32_t _speed, _position;
    pbio_control_settings_get_target_tolerances(&self->control->settings, &_speed, &_position);

    // If all given values are none, return current values
    if (speed == mp_const_none && position == mp_const_none) {
        mp_obj_t ret[2];
        ret[0] = mp_obj_new_int(_speed);
        ret[1] = mp_obj_new_int(_position);
        return mp_obj_new_tuple(2, ret);
    }

    // Assert control is not active
    raise_if_control_busy(self->control);

    // Set user settings
    _speed = pb_obj_get_default_int(speed, _speed);
    _position = pb_obj_get_default_int(position, _position);

    pb_assert(pbio_control_settings_set_target_tolerances(&self->control->settings, _speed, _position));

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
    int32_t _speed, _time;
    pbio_control_settings_get_stall_tolerances(&self->control->settings, &_speed, &_time);

    // If all given values are none, return current values
    if (speed == mp_const_none && time == mp_const_none) {
        mp_obj_t ret[2];
        ret[0] = mp_obj_new_int(_speed);
        ret[1] = mp_obj_new_int(_time);
        return mp_obj_new_tuple(2, ret);
    }

    // Assert control is not active
    raise_if_control_busy(self->control);

    // Set user settings
    _speed = pb_obj_get_default_int(speed, _speed);
    _time = pb_obj_get_default_int(time, _time);

    pb_assert(pbio_control_settings_set_stall_tolerances(&self->control->settings, _speed, _time));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_Control_stall_tolerances_obj, 1, common_Control_stall_tolerances);

// pybricks._common.Control.trajectory
STATIC mp_obj_t common_Control_trajectory(mp_obj_t self_in) {
    common_Control_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_trajectory_t trajectory;

    mp_obj_t parms[12];

    trajectory = self->control->trajectory;

    if (self->control->type != PBIO_CONTROL_NONE) {
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
    { MP_ROM_QSTR(MP_QSTR_stalled), MP_ROM_PTR(&common_Control_stalled_obj) },
    { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_ATTRIBUTE_OFFSET(common_Control_obj_t, scale) },
};
STATIC MP_DEFINE_CONST_DICT(common_Control_locals_dict, common_Control_locals_dict_table);

// type(pybricks.common.Control)
const mp_obj_type_t pb_type_Control = {
    { &mp_type_type },
    .name = MP_QSTR_Control,
    .locals_dict = (mp_obj_dict_t *)&common_Control_locals_dict,
};

#endif // PYBRICKS_PY_COMMON_MOTORS
