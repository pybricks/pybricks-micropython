// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "modmotor.h"
#include "py/mphal.h"
#include "py/runtime.h"

#include "pbobj.h"

#include <pbio/iodev.h>
#include <pbio/ev3device.h>
#include <pberror.h>

// pybricks.ev3devices.InfraredSensor class object
typedef struct _ev3devices_InfraredSensor_obj_t {
    mp_obj_base_t base;
    ev3_iodev_t *iodev;
} ev3devices_InfraredSensor_obj_t;

// pybricks.ev3devices.InfraredSensor.__init__
STATIC mp_obj_t ev3devices_InfraredSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    ev3devices_InfraredSensor_obj_t *self = m_new_obj(ev3devices_InfraredSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    pb_assert(ev3device_get_device(&self->iodev, mp_obj_get_int(args[0])));
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.InfraredSensor.__str__
STATIC void ev3devices_InfraredSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_InfraredSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.ev3devices.InfraredSensor.distance
STATIC mp_obj_t ev3devices_InfraredSensor_distance(mp_obj_t self_in) {
    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t distance;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_PROX, &distance));
    return mp_obj_new_int(distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_InfraredSensor_distance_obj, ev3devices_InfraredSensor_distance);

// pybricks.ev3devices.InfraredSensor.beacon
STATIC mp_obj_t ev3devices_InfraredSensor_beacon(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    ev3devices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    STATIC const mp_arg_t allowed_args[] = {
        PB_ARG_REQUIRED(channel),
    };
    PB_PARSE_ARGS_METHOD(args, n_args, pos_args, kw_args, allowed_args);

    mp_int_t channel = pb_obj_get_int(args[0].u_obj);

    if (channel < 1 || channel > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    int8_t beacon_data[8];
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_ID_EV3_IR_SENSOR__IR_SEEK, beacon_data));

    mp_int_t heading = beacon_data[channel*2-2]*3;
    mp_int_t distance = beacon_data[channel*2-1];
    
    mp_obj_t ret[2];
    
    if (distance == -128) {
        ret[0] = mp_const_none;
        ret[1] = mp_const_none;
    }
    else {
        ret[0] = mp_obj_new_int(distance);
        ret[1] = mp_obj_new_int(heading);
    }

    return mp_obj_new_tuple(2, ret);
}
MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_InfraredSensor_beacon_obj, 0, ev3devices_InfraredSensor_beacon);

// dir(pybricks.ev3devices.InfraredSensor)
STATIC const mp_rom_map_elem_t ev3devices_InfraredSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&ev3devices_InfraredSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_beacon),   MP_ROM_PTR(&ev3devices_InfraredSensor_beacon_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_InfraredSensor_locals_dict, ev3devices_InfraredSensor_locals_dict_table);

STATIC const mp_obj_type_t ev3devices_InfraredSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_InfraredSensor,
    .print = ev3devices_InfraredSensor_print,
    .make_new = ev3devices_InfraredSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_InfraredSensor_locals_dict,
};

// ColorSensor
typedef struct _ev3devices_ColorSensor_obj_t {
    mp_obj_base_t base;
    ev3_iodev_t *iodev;
} ev3devices_ColorSensor_obj_t;

STATIC mp_obj_t ev3devices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    ev3devices_ColorSensor_obj_t *self = m_new_obj(ev3devices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    pb_assert(ev3device_get_device(&self->iodev, mp_obj_get_int(args[0])));
    return MP_OBJ_FROM_PTR(self);
}

STATIC void ev3devices_ColorSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_ColorSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

STATIC mp_obj_t ev3devices_ColorSensor_reflection(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t reflection;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__COL_REFLECT, &reflection));
    return mp_obj_new_int(reflection);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_reflection_obj, ev3devices_ColorSensor_reflection);

STATIC mp_obj_t ev3devices_ColorSensor_rgb(mp_obj_t self_in) {
    ev3devices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t rgb[3];
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_ID_EV3_COLOR_SENSOR__RGB_RAW, rgb));
    mp_obj_t tup[3];

    rgb[0] = (0.258*rgb[0])-0.3;
    rgb[1] = (0.280*rgb[1])-0.8;
    rgb[2] = (0.523*rgb[2])-3.7;

    for (uint8_t i = 0; i < 3; i++) {
        rgb[i] = (rgb[i] > 100 ? 100 : rgb[i]);
        rgb[i] = (rgb[i] < 0   ?   0 : rgb[i]);
        tup[i] = mp_obj_new_int(rgb[i]);
    }
    return mp_obj_new_tuple(3, tup);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_ColorSensor_rgb_obj, ev3devices_ColorSensor_rgb);

STATIC const mp_rom_map_elem_t ev3devices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&ev3devices_ColorSensor_reflection_obj) },
    { MP_ROM_QSTR(MP_QSTR_rgb), MP_ROM_PTR(&ev3devices_ColorSensor_rgb_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_ColorSensor_locals_dict, ev3devices_ColorSensor_locals_dict_table);

STATIC const mp_obj_type_t ev3devices_ColorSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorSensor,
    .print = ev3devices_ColorSensor_print,
    .make_new = ev3devices_ColorSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3devices_ColorSensor_locals_dict,
};

STATIC const mp_rom_map_elem_t ev3devices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),       MP_ROM_QSTR(MP_QSTR_ev3devices)              },
    { MP_ROM_QSTR(MP_QSTR_Motor),          MP_ROM_PTR(&motor_Motor_type)                },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor), MP_ROM_PTR(&ev3devices_InfraredSensor_type)  },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),    MP_ROM_PTR(&ev3devices_ColorSensor_type)     },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_ev3devices_globals, ev3devices_globals_table);
const mp_obj_module_t pb_module_ev3devices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3devices_globals,
};
