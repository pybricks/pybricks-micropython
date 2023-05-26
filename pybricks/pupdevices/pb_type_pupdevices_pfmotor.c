// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbio/button.h>
#include <pbio/dcmotor.h>

#include "py/mphal.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_color_map.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for PFMotor.
typedef struct _pupdevices_PFMotor_obj_t {
    mp_obj_base_t base;
    // Point to ColorDistanceSensor, this class does not manage its own device.
    pb_pupdevices_obj_base_t *pup_base;
    uint8_t channel;
    bool use_blue_port;
    pbio_direction_t direction;
} pupdevices_PFMotor_obj_t;

// pybricks.pupdevices.PFMotor.__init__
STATIC mp_obj_t pupdevices_PFMotor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(sensor),
        PB_ARG_REQUIRED(channel),
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_OBJ(positive_direction, pb_Direction_CLOCKWISE_obj));

    // Get channel
    mp_int_t channel = mp_obj_get_int(channel_in);
    if (channel < 1 || channel > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get port color indicator (blue or red)
    uint16_t hue = pb_type_Color_get_hsv(color_in)->h;
    if (hue != PBIO_COLOR_HUE_BLUE && hue != PBIO_COLOR_HUE_RED) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    bool use_blue_port = hue == PBIO_COLOR_HUE_BLUE;

    // Get positive direction.
    pbio_direction_t positive_direction = pb_type_enum_get_value(positive_direction_in, &pb_enum_type_Direction);

    // All checks have passed, so create the object
    pupdevices_PFMotor_obj_t *self = mp_obj_malloc(pupdevices_PFMotor_obj_t, type);
    self->pup_base = pupdevices_ColorDistanceSensor__get_device(sensor_in);
    self->channel = channel;
    self->use_blue_port = use_blue_port;
    self->direction = positive_direction;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t pupdevices_PFMotor__send(pupdevices_PFMotor_obj_t *self, int16_t message) {
    // Choose blue or red output
    message |= (self->use_blue_port) << 4;

    // Choose single output Mode
    message |= 1 << 6;

    // Choose channel (1--4)
    message |= (self->channel - 1) << 8;

    // Send the data to the device. This automatically delays by about 250 ms
    // to ensure the data is properly sent and received. This also ensures that
    // the message will still work if two identical values are sent in a row.
    return pb_pupdevices_set_data(self->pup_base, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX, &message);
}

// pybricks.pupdevices.PFMotor.dc
STATIC mp_obj_t pupdevices_PFMotor_dc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_PFMotor_obj_t, self,
        PB_ARG_REQUIRED(duty));

    mp_int_t duty = pb_obj_get_int(duty_in);
    bool forward = (duty > 0) == (self->direction == PBIO_DIRECTION_CLOCKWISE);

    // Make duty cycle positive and bounded
    if (duty < 0) {
        duty = -duty;
    }
    if (duty > 99) {
        duty = 105;
    }

    // Scale 100% duty to 7 PWM steps
    uint8_t pwm = duty / 15;

    // // For forward, PWM steps 1--7 are binary 1 to 7, backward is 15--9
    int32_t message = (forward || pwm == 0) ? pwm : 16 - pwm;

    return pupdevices_PFMotor__send(self, message);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_PFMotor_dc_obj, 1, pupdevices_PFMotor_dc);

// pybricks.pupdevices.PFMotor.stop
STATIC mp_obj_t pupdevices_PFMotor_stop(mp_obj_t self_in) {
    return pupdevices_PFMotor__send(MP_OBJ_TO_PTR(self_in), 0);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_PFMotor_stop_obj, pupdevices_PFMotor_stop);

// pybricks.pupdevices.PFMotor.brake
STATIC mp_obj_t pupdevices_PFMotor_brake(mp_obj_t self_in) {
    return pupdevices_PFMotor__send(MP_OBJ_TO_PTR(self_in), 8);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_PFMotor_brake_obj, pupdevices_PFMotor_brake);

// dir(pybricks.pupdevices.PFMotor)
STATIC const mp_rom_map_elem_t pupdevices_PFMotor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_dc),      MP_ROM_PTR(&pupdevices_PFMotor_dc_obj)},
    { MP_ROM_QSTR(MP_QSTR_stop),    MP_ROM_PTR(&pupdevices_PFMotor_stop_obj)},
    { MP_ROM_QSTR(MP_QSTR_brake),   MP_ROM_PTR(&pupdevices_PFMotor_brake_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_PFMotor_locals_dict, pupdevices_PFMotor_locals_dict_table);

// type(pybricks.pupdevices.PFMotor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_PFMotor,
    MP_QSTR_PFMotor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_PFMotor_make_new,
    locals_dict, &pupdevices_PFMotor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
