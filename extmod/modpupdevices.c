// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbdrv/ioport.h>

#include <pbio/button.h>
#include <pbio/iodev.h>
#include <pbio/light.h>

#include "py/runtime.h"

#include "pbdevice.h"
#include "pberror.h"
#include "pbkwarg.h"

#include "modbuiltins.h"
#include "modparameters.h"
#include "modmotor.h"

// Class structure for ColorDistanceSensor
typedef struct _pupdevices_ColorDistanceSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    pbdevice_t *pbdev;
} pupdevices_ColorDistanceSensor_obj_t;

// pybricks.pupdevices.ColorDistanceSensor.__init__
STATIC mp_obj_t pupdevices_ColorDistanceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorDistanceSensor_obj_t *self = m_new_obj(pupdevices_ColorDistanceSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);

    // Create an instance of the Light class
    self->light = builtins_ColorLight_obj_make_new(self->pbdev);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ColorDistanceSensor.color
STATIC mp_obj_t pupdevices_ColorDistanceSensor_color(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t data[4];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1, data);

    switch (data[0]) {
        case 1:
            return pb_const_color_black;
        case 3:
            return pb_const_color_blue;
        case 5:
            return pb_const_color_green;
        case 7:
            return pb_const_color_yellow;
        case 8:
            return pb_const_color_orange;
        case 9:
            return pb_const_color_red;
        case 10:
            return pb_const_color_white;
        default:
            return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_color_obj, pupdevices_ColorDistanceSensor_color);

// pybricks.pupdevices.ColorDistanceSensor.distance
STATIC mp_obj_t pupdevices_ColorDistanceSensor_distance(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t data[4];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1, data);
    return mp_obj_new_int(data[1] * 10);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_distance_obj, pupdevices_ColorDistanceSensor_distance);

// pybricks.pupdevices.ColorDistanceSensor.reflection
STATIC mp_obj_t pupdevices_ColorDistanceSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t data[4];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__SPEC1, data);
    return mp_obj_new_int(data[3]);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_reflection_obj, pupdevices_ColorDistanceSensor_reflection);

// pybricks.pupdevices.ColorDistanceSensor.ambient
STATIC mp_obj_t pupdevices_ColorDistanceSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t ambient;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__AMBI, &ambient);
    return mp_obj_new_int(ambient);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_ambient_obj, pupdevices_ColorDistanceSensor_ambient);

// pybricks.pupdevices.ColorDistanceSensor.remote
STATIC mp_obj_t pupdevices_ColorDistanceSensor_remote(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorDistanceSensor_obj_t, self,
        PB_ARG_REQUIRED(channel),
        PB_ARG_DEFAULT_NONE(button_1),
        PB_ARG_DEFAULT_NONE(button_2));

    // Get channel
    mp_int_t ch = mp_obj_get_int(channel);
    if (ch < 1 || ch > 4) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get individual button codes
    pbio_button_flags_t b1, b2, btn;
    b1 = button_1 == mp_const_none ? 0 : pb_type_enum_get_value(button_1, &pb_enum_type_Button);
    b2 = button_2 == mp_const_none ? 0 : pb_type_enum_get_value(button_2, &pb_enum_type_Button);

    // Full button mask
    btn = b1 | b2;

    // Power Functions 1.0 "Combo Direct Mode" without checksum
    int32_t message = ((btn & PBIO_BUTTON_LEFT_UP) != 0) << 0 |
                ((btn & PBIO_BUTTON_LEFT_DOWN) != 0) << 1 |
                ((btn & PBIO_BUTTON_RIGHT_UP) != 0) << 2 |
                ((btn & PBIO_BUTTON_RIGHT_DOWN) != 0) << 3 |
                (1) << 4 |
                (ch - 1) << 8;

    // Send the data to the device
    pbdevice_set_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__IR_TX, &message, 1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorDistanceSensor_remote_obj, 1, pupdevices_ColorDistanceSensor_remote);

// pybricks.pupdevices.ColorDistanceSensor.rgb
STATIC mp_obj_t pupdevices_ColorDistanceSensor_rgb(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t data[3];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, data);

    mp_obj_t rgb[3];
    for (uint8_t col = 0; col < 3; col++) {
        int16_t intensity = (data[col] * 10) / 44;
        rgb[col] = mp_obj_new_int(intensity < 100 ? intensity : 100);
    }
    return mp_obj_new_tuple(3, rgb);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_rgb_obj, pupdevices_ColorDistanceSensor_rgb);

// dir(pybricks.pupdevices.ColorDistanceSensor)
STATIC const mp_rom_map_elem_t pupdevices_ColorDistanceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&pupdevices_ColorDistanceSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_ColorDistanceSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&pupdevices_ColorDistanceSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_ColorDistanceSensor_distance_obj)             },
    { MP_ROM_QSTR(MP_QSTR_remote),      MP_ROM_PTR(&pupdevices_ColorDistanceSensor_remote_obj)               },
    { MP_ROM_QSTR(MP_QSTR_rgb),         MP_ROM_PTR(&pupdevices_ColorDistanceSensor_rgb_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(pupdevices_ColorDistanceSensor_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorDistanceSensor_locals_dict, pupdevices_ColorDistanceSensor_locals_dict_table);

// type(pybricks.pupdevices.ColorDistanceSensor)
STATIC const mp_obj_type_t pupdevices_ColorDistanceSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorDistanceSensor,
    .make_new = pupdevices_ColorDistanceSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_ColorDistanceSensor_locals_dict,
};

// dir(pybricks.pupdevices)
STATIC const mp_rom_map_elem_t pupdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pupdevices)                     },
    { MP_ROM_QSTR(MP_QSTR_Motor),               MP_ROM_PTR(&motor_Motor_type)                    },
    { MP_ROM_QSTR(MP_QSTR_DCMotor),             MP_ROM_PTR(&motor_DCMotor_type)                  },
    { MP_ROM_QSTR(MP_QSTR_ColorDistanceSensor), MP_ROM_PTR(&pupdevices_ColorDistanceSensor_type) },
};

STATIC MP_DEFINE_CONST_DICT(
    pb_module_pupdevices_globals,
    pupdevices_globals_table);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_pupdevices_globals,
};

#endif // PYBRICKS_PY_PUPDEVICES
