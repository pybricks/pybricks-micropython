// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2020 The Pybricks Authors

#include <pbio/color.h>

#include <pbio/iodev.h>

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objtype.h"

#include "util_pb/pb_device.h"

#include "util_mp/pb_obj_helper.h"
#include "util_mp/pb_kwarg_helper.h"
#include "util_pb/pb_error.h"

#include "common/common.h"
#include "common/common_motors.h"

#include "modparameters.h"

#if PYBRICKS_HUB_EV3

// Generic linear scaling of an analog value between a known min and max to a percentage
STATIC int32_t analog_scale(int32_t mvolts, int32_t mvolts_min, int32_t mvolts_max, bool invert) {
    int32_t scaled = (100 * (mvolts - mvolts_min)) / (mvolts_max - mvolts_min);
    if (invert) {
        scaled = 100 - scaled;
    }
    return max(0, min(scaled, 100));
}

// pybricks.nxtdevices.UltrasonicSensor class object
typedef struct _nxtdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_UltrasonicSensor_obj_t;

// pybricks.nxtdevices.UltrasonicSensor.__init__
STATIC mp_obj_t nxtdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_UltrasonicSensor_obj_t *self = m_new_obj(nxtdevices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.UltrasonicSensor.distance
STATIC mp_obj_t nxtdevices_UltrasonicSensor_distance(mp_obj_t self_in) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t distance;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM, &distance);
    return mp_obj_new_int(distance * 10);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_UltrasonicSensor_distance_obj, nxtdevices_UltrasonicSensor_distance);

// dir(pybricks.nxtdevices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t nxtdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_distance_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_UltrasonicSensor_locals_dict, nxtdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.nxtdevices.UltrasonicSensor)
STATIC const mp_obj_type_t nxtdevices_UltrasonicSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_UltrasonicSensor,
    .make_new = nxtdevices_UltrasonicSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_UltrasonicSensor_locals_dict,
};

// pybricks.nxtdevices.TouchSensor class object
typedef struct _nxtdevices_TouchSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_TouchSensor_obj_t;

// pybricks.nxtdevices.TouchSensor.__init__
STATIC mp_obj_t nxtdevices_TouchSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_TouchSensor_obj_t *self = m_new_obj(nxtdevices_TouchSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.TouchSensor.pressed
STATIC mp_obj_t nxtdevices_TouchSensor_pressed(mp_obj_t self_in) {
    nxtdevices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_TOUCH_SENSOR__TOUCH, &analog);
    return mp_obj_new_bool(analog < 2500);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_TouchSensor_pressed_obj, nxtdevices_TouchSensor_pressed);

// dir(pybricks.ev3devices.TouchSensor)
STATIC const mp_rom_map_elem_t nxtdevices_TouchSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&nxtdevices_TouchSensor_pressed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_TouchSensor_locals_dict, nxtdevices_TouchSensor_locals_dict_table);

// type(pybricks.ev3devices.TouchSensor)
STATIC const mp_obj_type_t nxtdevices_TouchSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_TouchSensor,
    .make_new = nxtdevices_TouchSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_TouchSensor_locals_dict,
};

// pybricks.nxtdevices.SoundSensor class object
typedef struct _nxtdevices_SoundSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_SoundSensor_obj_t;

// pybricks.nxtdevices.SoundSensor.intensity
STATIC mp_obj_t nxtdevices_SoundSensor_intensity(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        nxtdevices_SoundSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(audible_only));

    uint8_t mode = mp_obj_is_true(audible_only) ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    int32_t analog;
    pb_device_get_values(self->pbdev, mode, &analog);

    return mp_obj_new_int(analog_scale(analog, 650, 4860, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(nxtdevices_SoundSensor_intensity_obj, 1, nxtdevices_SoundSensor_intensity);

// pybricks.nxtdevices.SoundSensor.__init__
STATIC mp_obj_t nxtdevices_SoundSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_SoundSensor_obj_t *self = m_new_obj(nxtdevices_SoundSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_SOUND_SENSOR);

    // Do one reading for consistent initial mode
    mp_obj_t pos_args[1] = { self };
    mp_map_t kwd_args;
    mp_map_init_fixed_table(&kwd_args, 0, NULL);
    nxtdevices_SoundSensor_intensity(1, pos_args, &kwd_args);

    return MP_OBJ_FROM_PTR(self);
}

// dir(pybricks.ev3devices.SoundSensor)
STATIC const mp_rom_map_elem_t nxtdevices_SoundSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_intensity),  MP_ROM_PTR(&nxtdevices_SoundSensor_intensity_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_SoundSensor_locals_dict, nxtdevices_SoundSensor_locals_dict_table);

// type(pybricks.ev3devices.SoundSensor)
STATIC const mp_obj_type_t nxtdevices_SoundSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_SoundSensor,
    .make_new = nxtdevices_SoundSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_SoundSensor_locals_dict,
};

// pybricks.nxtdevices.LightSensor class object
typedef struct _nxtdevices_LightSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_LightSensor_obj_t;

// pybricks.nxtdevices.LightSensor.ambient
STATIC mp_obj_t nxtdevices_LightSensor_ambient(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;

    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__AMBIENT, &analog);

    return mp_obj_new_int(analog_scale(analog, 1906, 4164, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_ambient_obj, nxtdevices_LightSensor_ambient);

// pybricks.nxtdevices.LightSensor.reflection
STATIC mp_obj_t nxtdevices_LightSensor_reflection(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;

    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__REFLECT, &analog);

    return mp_obj_new_int(analog_scale(analog, 1906, 3000, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_reflection_obj, nxtdevices_LightSensor_reflection);

// pybricks.nxtdevices.LightSensor.__init__
STATIC mp_obj_t nxtdevices_LightSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_LightSensor_obj_t *self = m_new_obj(nxtdevices_LightSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR);

    // Read one value to ensure a consistent initial mode
    nxtdevices_LightSensor_reflection(self);

    return MP_OBJ_FROM_PTR(self);
}

// dir(pybricks.ev3devices.LightSensor)
STATIC const mp_rom_map_elem_t nxtdevices_LightSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ambient),  MP_ROM_PTR(&nxtdevices_LightSensor_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_LightSensor_reflection_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_LightSensor_locals_dict, nxtdevices_LightSensor_locals_dict_table);

// type(pybricks.ev3devices.LightSensor)
STATIC const mp_obj_type_t nxtdevices_LightSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_LightSensor,
    .make_new = nxtdevices_LightSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_LightSensor_locals_dict,
};

// pybricks.nxtdevices.ColorSensor class object
typedef struct _nxtdevices_ColorSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    pb_device_t *pbdev;
} nxtdevices_ColorSensor_obj_t;

// pybricks.nxtdevices.ColorSensor.__init__
STATIC mp_obj_t nxtdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_ColorSensor_obj_t *self = m_new_obj(nxtdevices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR);

    // Create an instance of the Light class
    self->light = common_ColorLight_obj_make_new(self->pbdev);

    // Set the light color to red
    pb_device_color_light_on(self->pbdev, PBIO_COLOR_RED);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t color_obj(pbio_color_t color) {
    switch (color) {
        case PBIO_COLOR_RED:
            return pb_const_color_red;
        case PBIO_COLOR_GREEN:
            return pb_const_color_green;
        case PBIO_COLOR_BLUE:
            return pb_const_color_blue;
        case PBIO_COLOR_YELLOW:
            return pb_const_color_yellow;
        case PBIO_COLOR_BLACK:
            return pb_const_color_black;
        case PBIO_COLOR_WHITE:
            return pb_const_color_white;
        default:
            return mp_const_none;
    }
}

// pybricks.nxtdevices.ColorSensor.all
STATIC mp_obj_t nxtdevices_ColorSensor_all(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    mp_obj_t ret[5];
    for (uint8_t i = 0; i < 4; i++) {
        ret[i] = mp_obj_new_int(all[i]);
    }
    ret[4] = color_obj(all[4]);

    return mp_obj_new_tuple(5, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_all_obj, nxtdevices_ColorSensor_all);

// pybricks.nxtdevices.ColorSensor.rgb
STATIC mp_obj_t nxtdevices_ColorSensor_rgb(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    mp_obj_t ret[3];
    for (uint8_t i = 0; i < 3; i++) {
        ret[i] = mp_obj_new_int(all[i]);
    }
    return mp_obj_new_tuple(3, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_rgb_obj, nxtdevices_ColorSensor_rgb);

// pybricks.nxtdevices.ColorSensor.reflection
STATIC mp_obj_t nxtdevices_ColorSensor_reflection(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the average of red, green, and blue reflection
    return mp_obj_new_int((all[0] + all[1] + all[2]) / 3);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_reflection_obj, nxtdevices_ColorSensor_reflection);

// pybricks.nxtdevices.ColorSensor.ambient
STATIC mp_obj_t nxtdevices_ColorSensor_ambient(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the ambient light
    return mp_obj_new_int(all[3]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_ambient_obj, nxtdevices_ColorSensor_ambient);

// pybricks.nxtdevices.ColorSensor.color
STATIC mp_obj_t nxtdevices_ColorSensor_color(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[5];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the color ID
    return color_obj(all[4]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_color_obj, nxtdevices_ColorSensor_color);

// dir(pybricks.nxtdevices.ColorSensor)
STATIC const mp_rom_map_elem_t nxtdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_all),        MP_ROM_PTR(&nxtdevices_ColorSensor_all_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_rgb),        MP_ROM_PTR(&nxtdevices_ColorSensor_rgb_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_ambient),    MP_ROM_PTR(&nxtdevices_ColorSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_ColorSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_color),      MP_ROM_PTR(&nxtdevices_ColorSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_light),      MP_ROM_ATTRIBUTE_OFFSET(nxtdevices_ColorSensor_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_ColorSensor_locals_dict, nxtdevices_ColorSensor_locals_dict_table);

// type(pybricks.nxtdevices.ColorSensor)
STATIC const mp_obj_type_t nxtdevices_ColorSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorSensor,
    .make_new = nxtdevices_ColorSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_ColorSensor_locals_dict,
};

// pybricks.nxtdevices.TemperatureSensor class object
typedef struct _nxtdevices_TemperatureSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_TemperatureSensor_obj_t;

// pybricks.nxtdevices.TemperatureSensor.__init__
STATIC mp_obj_t nxtdevices_TemperatureSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_TemperatureSensor_obj_t *self = m_new_obj(nxtdevices_TemperatureSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_TEMPERATURE_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.TemperatureSensor.temperature
STATIC mp_obj_t nxtdevices_TemperatureSensor_temperature(mp_obj_t self_in) {
    nxtdevices_TemperatureSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t temperature_scaled;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_TEMPERATURE_SENSOR_CELCIUS, &temperature_scaled);
    return mp_obj_new_float((temperature_scaled >> 4) / 16.0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_TemperatureSensor_temperature_obj, nxtdevices_TemperatureSensor_temperature);

// dir(pybricks.ev3devices.TemperatureSensor)
STATIC const mp_rom_map_elem_t nxtdevices_TemperatureSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_temperature),    MP_ROM_PTR(&nxtdevices_TemperatureSensor_temperature_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_TemperatureSensor_locals_dict, nxtdevices_TemperatureSensor_locals_dict_table);

// type(pybricks.nxtdevices.TemperatureSensor)
STATIC const mp_obj_type_t nxtdevices_TemperatureSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_TemperatureSensor,
    .make_new = nxtdevices_TemperatureSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_TemperatureSensor_locals_dict,
};

// pybricks.nxtdevices.EnergyMeter class object
typedef struct _nxtdevices_EnergyMeter_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_EnergyMeter_obj_t;

// pybricks.nxtdevices.EnergyMeter.__init__
STATIC mp_obj_t nxtdevices_EnergyMeter_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_EnergyMeter_obj_t *self = m_new_obj(nxtdevices_EnergyMeter_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_ENERGY_METER);

    // Read once so we are in the mode we'll be using for all methods, to avoid mode switch delays later
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.EnergyMeter.storage
STATIC mp_obj_t nxtdevices_EnergyMeter_storage(mp_obj_t self_in) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);
    return mp_obj_new_int(all[4]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_storage_obj, nxtdevices_EnergyMeter_storage);

// pybricks.nxtdevices.EnergyMeter.input
STATIC mp_obj_t nxtdevices_EnergyMeter_input(mp_obj_t self_in) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int(all[0]);
    dat[1] = mp_obj_new_int(all[1]);
    dat[2] = mp_obj_new_int(all[5]);
    return mp_obj_new_tuple(3, dat);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_input_obj, nxtdevices_EnergyMeter_input);

// pybricks.nxtdevices.EnergyMeter.output
STATIC mp_obj_t nxtdevices_EnergyMeter_output(mp_obj_t self_in) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int(all[2]);
    dat[1] = mp_obj_new_int(all[3]);
    dat[2] = mp_obj_new_int(all[6]);
    return mp_obj_new_tuple(3, dat);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_output_obj, nxtdevices_EnergyMeter_output);

// dir(pybricks.ev3devices.EnergyMeter)
STATIC const mp_rom_map_elem_t nxtdevices_EnergyMeter_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_input),      MP_ROM_PTR(&nxtdevices_EnergyMeter_input_obj) },
    { MP_ROM_QSTR(MP_QSTR_output),     MP_ROM_PTR(&nxtdevices_EnergyMeter_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_storage),    MP_ROM_PTR(&nxtdevices_EnergyMeter_storage_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_EnergyMeter_locals_dict, nxtdevices_EnergyMeter_locals_dict_table);

// type(pybricks.nxtdevices.EnergyMeter)
STATIC const mp_obj_type_t nxtdevices_EnergyMeter_type = {
    { &mp_type_type },
    .name = MP_QSTR_EnergyMeter,
    .make_new = nxtdevices_EnergyMeter_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_EnergyMeter_locals_dict,
};

#endif // PYBRICKS_HUB_EV3

// dir(pybricks.nxtdevices)
STATIC const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_nxtdevices)              },
    #if PYBRICKS_HUB_NXT
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&pb_type_Motor)                },
    #else
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),      MP_ROM_PTR(&nxtdevices_TouchSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_SoundSensor),      MP_ROM_PTR(&nxtdevices_SoundSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_LightSensor),      MP_ROM_PTR(&nxtdevices_LightSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_type)},
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),      MP_ROM_PTR(&nxtdevices_ColorSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_TemperatureSensor),MP_ROM_PTR(&nxtdevices_TemperatureSensor_type)},
    { MP_ROM_QSTR(MP_QSTR_EnergyMeter),      MP_ROM_PTR(&nxtdevices_EnergyMeter_type)     },
    #endif
};

STATIC MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_nxtdevices_globals,
};

