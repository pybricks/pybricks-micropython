// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/control.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"
#include "py/mphal.h"

#include "modbuiltins.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.builtins.Control class object structure
typedef struct _builtins_Control_obj_t {
    mp_obj_base_t base;
    pbio_control_t *control;
} builtins_Control_obj_t;

// pybricks.builtins.Control.__init__/__new__
mp_obj_t builtins_Control_obj_make_new(pbio_control_t *control) {

    builtins_Control_obj_t *self = m_new_obj(builtins_Control_obj_t);
    self->base.type = &builtins_Control_type;

    self->control = control;
    
    return self;
}

// pybricks.builtins.Control.limits
STATIC mp_obj_t builtins_Control_limits(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        builtins_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(speed),
        PB_ARG_DEFAULT_NONE(acceleration),
        PB_ARG_DEFAULT_NONE(actuation)
    );

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

    // Set user settings
    _speed = pb_obj_get_default_int(speed, _speed);
    _acceleration = pb_obj_get_default_int(acceleration, _acceleration);
    _actuation = pb_obj_get_default_int(actuation, _actuation);

    pbio_control_settings_set_limits(&self->control->settings, _speed, _acceleration, _actuation);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Control_limits_obj, 0, builtins_Control_limits);

// pybricks.builtins.Control.pid
STATIC mp_obj_t builtins_Control_pid(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        builtins_Control_obj_t, self,
        PB_ARG_DEFAULT_NONE(kp),
        PB_ARG_DEFAULT_NONE(ki),
        PB_ARG_DEFAULT_NONE(kd)
    );

    // Read current values
    int16_t _kp, _ki, _kd;
    pbio_control_settings_get_pid(&self->control->settings, &_kp, &_ki, &_kd);

    // If all given values are none, return current values
    if (kp == mp_const_none && ki == mp_const_none && kd == mp_const_none) {
        mp_obj_t ret[3];
        ret[0] = mp_obj_new_int(_kp);
        ret[1] = mp_obj_new_int(_ki);
        ret[2] = mp_obj_new_int(_kd);
        return mp_obj_new_tuple(3, ret);
    }

    // Set user settings
    _kp = pb_obj_get_default_int(kp, _kp);
    _ki = pb_obj_get_default_int(ki, _ki);
    _kd = pb_obj_get_default_int(kd, _kd);

    pbio_control_settings_set_pid(&self->control->settings, _kp, _ki, _kd);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Control_pid_obj, 0, builtins_Control_pid);

// dir(pybricks.builtins.Control)
STATIC const mp_rom_map_elem_t builtins_Control_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_limits ), MP_ROM_PTR(&builtins_Control_limits_obj) },
    { MP_ROM_QSTR(MP_QSTR_pid    ), MP_ROM_PTR(&builtins_Control_pid_obj   ) },
};
STATIC MP_DEFINE_CONST_DICT(builtins_Control_locals_dict, builtins_Control_locals_dict_table);

// type(pybricks.builtins.Control)
const mp_obj_type_t builtins_Control_type = {
    { &mp_type_type },
    .name = MP_QSTR_Control,
    .locals_dict = (mp_obj_dict_t*)&builtins_Control_locals_dict,
};
