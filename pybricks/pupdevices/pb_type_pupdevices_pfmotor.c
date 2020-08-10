// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbio/button.h>

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for PFMotor.
typedef struct _pupdevices_PFMotor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
    uint8_t channel;
    pbio_color_t color;
    pbio_direction_t direction;
} pupdevices_PFMotor_obj_t;

// pybricks.pupdevices.PFMotor.__init__
STATIC mp_obj_t pupdevices_PFMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(sensor),
        PB_ARG_REQUIRED(channel),
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_OBJ(positive_direction, pb_Direction_CLOCKWISE_obj));

    // Get device
    pb_device_t *pbdev = pupdevices_ColorDistanceSensor__get_device(sensor);

    // Get channel
    mp_int_t _channel = mp_obj_get_int(channel);
    if (_channel < 1 || _channel > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get port color indicator (blue or red)
    pbio_color_t _color = pb_type_enum_get_value(color, &pb_enum_type_Color);
    if (_color != PBIO_COLOR_BLUE && _color != PBIO_COLOR_RED) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get positive direction.
    pbio_direction_t _direction = pb_type_enum_get_value(positive_direction, &pb_enum_type_Direction);

    // All checks have passed, so create the object
    pupdevices_PFMotor_obj_t *self = m_new_obj(pupdevices_PFMotor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    // Save init arguments
    self->pbdev = pbdev;
    self->channel = _channel;
    self->color = _color;
    self->direction = _direction;

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.PFMotor.dc
STATIC mp_obj_t pupdevices_PFMotor_dc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_PFMotor_obj_t, self,
        PB_ARG_REQUIRED(duty));

    mp_int_t duty_cycle = pb_obj_get_int(duty);
    bool forward = (duty_cycle > 0) == (self->direction == PBIO_DIRECTION_CLOCKWISE);

    // Make duty cycle positive and bounded
    if (duty_cycle < 0) {
        duty_cycle = -duty_cycle;
    }
    if (duty_cycle > 99) {
        duty_cycle = 105;
    }

    // Scale 100% duty to 7 PWM steps
    uint8_t pwm = duty_cycle / 15;

    // // For forward, PWM steps 1--7 are binary 1 to 7, backward is 15--9
    int32_t message = forward ? pwm : 16 - pwm;

    // Choose blue or red output
    message |= (self->color == PBIO_COLOR_BLUE) << 4;

    // Choose single output Mode
    message |= 1 << 6;

    // Choose channel (1--4)
    message |= (self->channel - 1) << 8;

    // Send the data to the device
    pb_device_set_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX, &message, 1);

    // We need about 20 ms delay for decent enough signal transmission
    mp_hal_delay_ms(20);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_PFMotor_dc_obj, 1, pupdevices_PFMotor_dc);


// dir(pybricks.pupdevices.PFMotor)
STATIC const mp_rom_map_elem_t pupdevices_PFMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_dc),      MP_ROM_PTR(&pupdevices_PFMotor_dc_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_PFMotor_locals_dict, pupdevices_PFMotor_locals_dict_table);

// type(pybricks.pupdevices.PFMotor)
const mp_obj_type_t pb_type_pupdevices_PFMotor = {
    { &mp_type_type },
    .name = MP_QSTR_PFMotor,
    .make_new = pupdevices_PFMotor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_PFMotor_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
