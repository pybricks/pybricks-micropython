// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_MOTORS

#include <pbio/battery.h>

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

static pbio_dcmotor_t *get_dcmotor_from_object(mp_obj_t self_in) {
    if (mp_obj_is_type(self_in, &pb_type_Motor)) {
        return ((common_Motor_obj_t *)MP_OBJ_TO_PTR(self_in))->srv->dcmotor;
    }
    return ((common_DCMotor_obj_t *)MP_OBJ_TO_PTR(self_in))->dcmotor;
}

// pybricks._common.DCMotor.__init__
STATIC mp_obj_t common_DCMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_OBJ(positive_direction, pb_Direction_CLOCKWISE_obj));

    // Configure the motor with the selected arguments at pbio level
    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbio_direction_t direction = pb_type_enum_get_value(positive_direction_in, &pb_enum_type_Direction);

    #if PYBRICKS_PY_EV3DEVICES
    #include <pbdrv/motor.h>
    pbio_error_t err;
    while ((err = pbdrv_motor_setup(port, false)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(2000);
    }
    pb_assert(err);
    #endif

    // Get and initialize DC Motor
    pbio_dcmotor_t *dcmotor;
    pb_assert(pbio_dcmotor_get_dcmotor(port, &dcmotor));
    pb_assert(pbio_dcmotor_setup(dcmotor, direction));

    // On success, create and return the MicroPython object
    common_DCMotor_obj_t *self = m_new_obj(common_DCMotor_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->dcmotor = dcmotor;
    return MP_OBJ_FROM_PTR(self);
}

// pybricks._common.DCMotor.__repr__
// pybricks._common.Motor.__repr__
void common_DCMotor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {

    // Get the dcmotor from self, which is either Motor or DCMotor
    pbio_dcmotor_t *dcmotor = get_dcmotor_from_object(self_in);

    mp_printf(print, "%q(Port.%c, %q.%q)",
        ((mp_obj_base_t *)MP_OBJ_TO_PTR(self_in))->type->name,
        dcmotor->port, MP_QSTR_Direction,
        dcmotor->direction == PBIO_DIRECTION_CLOCKWISE ? MP_QSTR_CLOCKWISE : MP_QSTR_COUNTERCLOCKWISE);
}

// pybricks._common.DCMotor.dc
// pybricks._common.Motor.dc
STATIC mp_obj_t common_DCMotor_duty(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse all arguments except the first one (self)
    PB_PARSE_ARGS_METHOD_SKIP_SELF(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(duty));

    // pbio has only voltage setters now, but the .dc() method will continue to
    // exist for backwards compatibility. So, we convert duty cycle to voltages.
    int32_t voltage = pbio_battery_get_voltage_from_duty(pb_obj_get_int(duty_in) * 100);
    pb_assert(pbio_dcmotor_set_voltage(get_dcmotor_from_object(pos_args[0]), voltage, true));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(common_DCMotor_duty_obj, 1, common_DCMotor_duty);

// pybricks._common.DCMotor.stop
// pybricks._common.Motor.stop
STATIC mp_obj_t common_DCMotor_stop(mp_obj_t self_in) {
    pb_assert(pbio_dcmotor_coast(get_dcmotor_from_object(self_in), true));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(common_DCMotor_stop_obj, common_DCMotor_stop);

// pybricks._common.DCMotor.brake
// pybricks._common.Motor.brake
STATIC mp_obj_t common_DCMotor_brake(mp_obj_t self_in) {
    pb_assert(pbio_dcmotor_set_voltage(get_dcmotor_from_object(self_in), 0, true));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(common_DCMotor_brake_obj, common_DCMotor_brake);

// pybricks._common.DCMotor.dc
// pybricks._common.Motor.dc
STATIC mp_obj_t common_DCMotor_dc_settings(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse all arguments except the first one (self)
    PB_PARSE_ARGS_METHOD_SKIP_SELF(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_NONE(max_voltage));

    // Get dcmotor from object
    pbio_dcmotor_t *dcmotor = get_dcmotor_from_object(pos_args[0]);

    // If no arguments given, return existing values
    if (max_voltage_in == mp_const_none) {
        int32_t max_voltage_now;
        pbio_dcmotor_get_settings(dcmotor, &max_voltage_now);
        mp_obj_t retval[1];
        retval[0] = mp_obj_new_int(max_voltage_now);
        return mp_obj_new_tuple(1, retval);
    }

    // Set the new limit
    pb_assert(pbio_dcmotor_set_settings(dcmotor, pb_obj_get_int(max_voltage_in)));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(common_DCMotor_dc_settings_obj, 1, common_DCMotor_dc_settings);

// dir(pybricks.builtins.DCMotor)
STATIC const mp_rom_map_elem_t common_DCMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_dc), MP_ROM_PTR(&common_DCMotor_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&common_DCMotor_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_brake), MP_ROM_PTR(&common_DCMotor_brake_obj) },
    { MP_ROM_QSTR(MP_QSTR_settings), MP_ROM_PTR(&common_DCMotor_dc_settings_obj) },
};
MP_DEFINE_CONST_DICT(common_DCMotor_locals_dict, common_DCMotor_locals_dict_table);

// type(pybricks.builtins.DCMotor)
const mp_obj_type_t pb_type_DCMotor = {
    { &mp_type_type },
    .name = MP_QSTR_DCMotor,
    .print = common_DCMotor_print,
    .make_new = common_DCMotor_make_new,
    .locals_dict = (mp_obj_dict_t *)&common_DCMotor_locals_dict,
};

#endif // PYBRICKS_PY_COMMON_MOTORS
