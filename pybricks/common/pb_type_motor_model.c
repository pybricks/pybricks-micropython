// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_MOTOR_MODEL && MICROPY_PY_BUILTINS_FLOAT

#include <pbio/observer.h>

#include "py/obj.h"

#include <pybricks/common.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

// pybricks._common.MotorModel class object structure
typedef struct _pb_type_MotorModel_obj_t {
    mp_obj_base_t base;
    pbio_observer_t *observer;
} pb_type_MotorModel_obj_t;

// pybricks._common.MotorModel.__init__/__new__
mp_obj_t pb_type_MotorModel_obj_make_new(pbio_observer_t *observer) {
    pb_type_MotorModel_obj_t *self = mp_obj_malloc(pb_type_MotorModel_obj_t, &pb_type_MotorModel);
    self->observer = observer;
    return MP_OBJ_FROM_PTR(self);
}

// pybricks._common.MotorModel.settings
static mp_obj_t pb_type_MotorModel_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_MotorModel_obj_t, self,
        PB_ARG_DEFAULT_NONE(values));

    // If all given values are none, return current values.
    if (values_in == mp_const_none) {
        mp_obj_t get_values[] = {
            mp_obj_new_int(self->observer->settings.stall_speed_limit),
            mp_obj_new_int(self->observer->settings.stall_time),
            mp_obj_new_int(self->observer->settings.feedback_voltage_negligible),
            mp_obj_new_int(self->observer->settings.feedback_voltage_stall_ratio),
            mp_obj_new_int(self->observer->settings.feedback_gain_low),
            mp_obj_new_int(self->observer->settings.feedback_gain_high),
            mp_obj_new_int(self->observer->settings.feedback_gain_threshold),
            mp_obj_new_int(self->observer->settings.coulomb_friction_speed_cutoff),
        };
        return mp_obj_new_tuple(MP_ARRAY_SIZE(get_values), get_values);
    }

    // Otherwise, unpack values and set them.
    size_t size;
    mp_obj_t *set_values;
    mp_obj_get_array(values_in, &size, &set_values);
    if (size != 8) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    self->observer->settings.stall_speed_limit = mp_obj_get_int(set_values[0]);
    self->observer->settings.stall_time = mp_obj_get_int(set_values[1]);
    self->observer->settings.feedback_voltage_negligible = mp_obj_get_int(set_values[2]);
    self->observer->settings.feedback_voltage_stall_ratio = mp_obj_get_int(set_values[3]);
    self->observer->settings.feedback_gain_low = mp_obj_get_int(set_values[4]);
    self->observer->settings.feedback_gain_high = mp_obj_get_int(set_values[5]);
    self->observer->settings.feedback_gain_threshold = mp_obj_get_int(set_values[6]);
    self->observer->settings.coulomb_friction_speed_cutoff = mp_obj_get_int(set_values[7]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_MotorModel_settings_obj, 1, pb_type_MotorModel_settings);

// pybricks._common.Motor.state
static mp_obj_t pb_type_MotorModel_state(mp_obj_t self_in) {
    pb_type_MotorModel_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_angle_t zero = {.millidegrees = 0, .rotations = 0};

    // return mp_obj_new_int(angle);
    mp_obj_t state[] = {
        mp_obj_new_float_from_f(pbio_angle_diff_mdeg(&self->observer->angle, &zero) / 1000.0f),
        mp_obj_new_float(self->observer->speed / 1000.0f),
        mp_obj_new_float(self->observer->current / 10.0f),
        mp_obj_new_bool(self->observer->stalled),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(state), state);
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_MotorModel_state_obj, pb_type_MotorModel_state);

// dir(pybricks.common.MotorModel)
static const mp_rom_map_elem_t pb_type_MotorModel_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_state),    MP_ROM_PTR(&pb_type_MotorModel_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&pb_type_MotorModel_settings_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_MotorModel_locals_dict, pb_type_MotorModel_locals_dict_table);

// type(pybricks.common.MotorModel)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_MotorModel,
    MP_QSTR_MotorModel,
    MP_TYPE_FLAG_NONE,
    locals_dict, &pb_type_MotorModel_locals_dict);

#endif // PYBRICKS_PY_COMMON_MOTOR_MODEL && MICROPY_PY_BUILTINS_FLOAT
