// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"
#include "py/mphal.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbdrv/ioport.h>

#include <pbio/button.h>
#include <pbio/iodev.h>
#include <pbio/light.h>
#include <pbio/math.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "pbdevice.h"
#include "pberror.h"
#include "pbkwarg.h"
#include "pbobj.h"
#include "pbhsv.h"

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
    int32_t distance;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__PROX, &distance);
    return mp_obj_new_int(distance * 10);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_distance_obj, pupdevices_ColorDistanceSensor_distance);

// pybricks.pupdevices.ColorDistanceSensor.reflection
STATIC mp_obj_t pupdevices_ColorDistanceSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t rgb[3];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, rgb);
    return mp_obj_new_int((rgb[0] + rgb[1] + rgb[2]) / 12);
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

// pybricks.pupdevices.ColorDistanceSensor.hsv
STATIC mp_obj_t pupdevices_ColorDistanceSensor_hsv(mp_obj_t self_in) {
    pupdevices_ColorDistanceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int32_t rgb[3];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_DISTANCE_SENSOR__RGB_I, rgb);

    // Make the algorithm below a bit easier to read
    #define COL_R (rgb[0])
    #define COL_G (rgb[1])
    #define COL_B (rgb[2])

    // Get maximum intensity
    int32_t max = COL_R > COL_G ? COL_R : COL_G;
    max = COL_B > max ? COL_B : max;

    // Get minimum intensity
    int32_t min = COL_R < COL_G ? COL_R : COL_G;
    min = COL_B < min ? COL_B : min;

    // Chroma
    int32_t chroma = max - min;
    int32_t hue = 0;

    // Get saturation as approximate percentage
    int32_t value = max/4;
    int32_t saturation;

    // Compute hue and saturation only if chroma is big enough
    if (chroma < 30) {
        hue = 0;
        saturation = 0;
    }
    else {
        // Get hue; chroma is always > 0 if we are here
        if (max == COL_R) {
            hue = (60*(COL_G - COL_B)) / chroma;
        }   
        else if (max == COL_G) {
            hue = (60*(COL_B - COL_R)) / chroma + 120;   
        }
        else if (max == COL_B) {
            hue = (60*(COL_R - COL_G)) / chroma + 240;
        }
        if (hue < 0) {
            hue += 360;
        }
        // Get saturation; max is always > 0 if we are here
        saturation = (100 * chroma) / max;
    }

    // Return hsv
    mp_obj_t hsv[3];
    hsv[0] = mp_obj_new_int(hue);
    hsv[1] = mp_obj_new_int(saturation);
    hsv[2] = mp_obj_new_int(value);
    return mp_obj_new_tuple(3, hsv);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorDistanceSensor_hsv_obj, pupdevices_ColorDistanceSensor_hsv);

// dir(pybricks.pupdevices.ColorDistanceSensor)
STATIC const mp_rom_map_elem_t pupdevices_ColorDistanceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&pupdevices_ColorDistanceSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_ColorDistanceSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&pupdevices_ColorDistanceSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_ColorDistanceSensor_distance_obj)             },
    { MP_ROM_QSTR(MP_QSTR_remote),      MP_ROM_PTR(&pupdevices_ColorDistanceSensor_remote_obj)               },
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&pupdevices_ColorDistanceSensor_hsv_obj)                  },
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

// Class structure for ColorSensor
typedef struct _pupdevices_ColorSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t lights;
    pbdevice_t *pbdev;
    pb_hsv_map_t color_map;
} pupdevices_ColorSensor_obj_t;

// pybricks.builtins.ColorSensor._get_hsv
STATIC void pupdevices_ColorSensor__get_hsv(pbdevice_t *pbdev, bool light_on, int32_t *hsv) {

    // Read HSV (light on) or SHSV mode (light off)
    pbdevice_get_values(pbdev, light_on ? PBIO_IODEV_MODE_PUP_COLOR_SENSOR__HSV : PBIO_IODEV_MODE_PUP_COLOR_SENSOR__SHSV, hsv);

    // Scale saturation and value to match 0-100% range in typical applications.
    // However, do not cap to allow other applications as well. For example, full sunlight will exceed 100%.
    if (light_on) {
        hsv[1] /= 8;
        hsv[2] /= 5;
    } else {
        hsv[1] /= 12;
        hsv[2] /= 12;
    }
}

// pybricks.pupdevices.ColorSensor.__init__
STATIC mp_obj_t pupdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ColorSensor_obj_t *self = m_new_obj(pupdevices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_SPIKE_COLOR_SENSOR);

    // This sensor requires power, which iodevice does not do automatically yet
    pbdevice_set_power_supply(self->pbdev, 100);

    // Create an instance of the LightArray class
    self->lights = builtins_LightArray_obj_make_new(self->pbdev, PBIO_IODEV_MODE_PUP_COLOR_SENSOR__LIGHT, 3);

    // Do one reading to make sure everything is working and to set default mode
    int32_t hsv[4];
    pupdevices_ColorSensor__get_hsv(self->pbdev, true, hsv);

    // Save default settings
    pb_hsv_map_save_default(&self->color_map);

    // This sensor needs some time to get values right after turning power on
    mp_hal_delay_ms(1000);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.builtins.ColorSensor.hsv
STATIC mp_obj_t pupdevices_ColorSensor_hsv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    // Read HSV, either with light on or off
    int32_t hsv[4];
    pupdevices_ColorSensor__get_hsv(self->pbdev, mp_obj_is_true(surface), hsv);

    // Create tuple
    mp_obj_t ret[3];
    ret[0] = mp_obj_new_int(hsv[0]);
    ret[1] = mp_obj_new_int(hsv[1]);
    ret[2] = mp_obj_new_int(hsv[2]);

    // Return hsv tuple
    return mp_obj_new_tuple(3, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorSensor_hsv_obj, 1, pupdevices_ColorSensor_hsv);

// pybricks.builtins.ColorSensor.color
STATIC mp_obj_t pupdevices_ColorSensor_color(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(surface));

    // Read HSV, either with light on or off
    int32_t hsv[4];
    pupdevices_ColorSensor__get_hsv(self->pbdev, mp_obj_is_true(surface), hsv);

    // Get and return discretized color
    return pb_hsv_get_color(&self->color_map, hsv[0], hsv[1], hsv[2]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorSensor_color_obj, 1, pupdevices_ColorSensor_color);

// pybricks.builtins.ColorSensor.color_map
STATIC mp_obj_t pupdevices_ColorSensor_color_map(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ColorSensor_obj_t, self,
        PB_ARG_DEFAULT_NONE(hues),
        PB_ARG_DEFAULT_NONE(saturation),
        PB_ARG_DEFAULT_NONE(values));

    // If no arguments are given, return current map
    if (hues == mp_const_none && saturation == mp_const_none && values == mp_const_none) {
        return pack_color_map(&self->color_map);
    }

    // Otherwise, unpack given map
    unpack_color_map(&self->color_map, hues, saturation, values);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ColorSensor_color_map_obj, 1, pupdevices_ColorSensor_color_map);

// pybricks.pupdevices.ColorSensor.reflection
STATIC mp_obj_t pupdevices_ColorSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Read HSV with light on
    int32_t hsv[4];
    pupdevices_ColorSensor__get_hsv(self->pbdev, true, hsv);

    // Return value as reflection
    return mp_obj_new_int(hsv[2]);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorSensor_reflection_obj, pupdevices_ColorSensor_reflection);

// pybricks.pupdevices.ColorSensor.ambient
STATIC mp_obj_t pupdevices_ColorSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Read HSV with light off
    int32_t hsv[4];
    pupdevices_ColorSensor__get_hsv(self->pbdev, false, hsv);

    // Return value as reflection
    return mp_obj_new_int(hsv[2]);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorSensor_ambient_obj, pupdevices_ColorSensor_ambient);

// dir(pybricks.pupdevices.ColorSensor)
STATIC const mp_rom_map_elem_t pupdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_lights),      MP_ROM_ATTRIBUTE_OFFSET(pupdevices_ColorSensor_obj_t, lights)},
    { MP_ROM_QSTR(MP_QSTR_hsv),         MP_ROM_PTR(&pupdevices_ColorSensor_hsv_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_color),       MP_ROM_PTR(&pupdevices_ColorSensor_color_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_ColorSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_ambient),     MP_ROM_PTR(&pupdevices_ColorSensor_ambient_obj)              },
    { MP_ROM_QSTR(MP_QSTR_color_map),   MP_ROM_PTR(&pupdevices_ColorSensor_color_map_obj)            },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorSensor_locals_dict, pupdevices_ColorSensor_locals_dict_table);

// type(pybricks.pupdevices.ColorSensor)
STATIC const mp_obj_type_t pupdevices_ColorSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorSensor,
    .make_new = pupdevices_ColorSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_ColorSensor_locals_dict,
};

// Class structure for UltrasonicSensor
typedef struct _pupdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t lights;
    pbdevice_t *pbdev;
} pupdevices_UltrasonicSensor_obj_t;

// pybricks.pupdevices.UltrasonicSensor.__init__
STATIC mp_obj_t pupdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_UltrasonicSensor_obj_t *self = m_new_obj(pupdevices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR);

    // This sensor requires power, which iodevice does not do automatically yet
    pbdevice_set_power_supply(self->pbdev, 100);

    // Create an instance of the LightArray class
    self->lights = builtins_LightArray_obj_make_new(self->pbdev, PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__LIGHT, 4);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.UltrasonicSensor.distance
STATIC mp_obj_t pupdevices_UltrasonicSensor_distance(mp_obj_t self_in) {
    pupdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t distance;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL, &distance);
    return mp_obj_new_int(distance < 0 ? 3000 : distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_UltrasonicSensor_distance_obj, pupdevices_UltrasonicSensor_distance);

// pybricks.pupdevices.UltrasonicSensor.presence
STATIC mp_obj_t pupdevices_UltrasonicSensor_presence(mp_obj_t self_in) {
    pupdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t presence;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__LISTN, &presence);
    return mp_obj_new_bool(presence);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_UltrasonicSensor_presence_obj, pupdevices_UltrasonicSensor_presence);

// dir(pybricks.pupdevices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t pupdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance),     MP_ROM_PTR(&pupdevices_UltrasonicSensor_distance_obj)              },
    { MP_ROM_QSTR(MP_QSTR_presence),     MP_ROM_PTR(&pupdevices_UltrasonicSensor_presence_obj)              },
    { MP_ROM_QSTR(MP_QSTR_lights),       MP_ROM_ATTRIBUTE_OFFSET(pupdevices_UltrasonicSensor_obj_t, lights) },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_UltrasonicSensor_locals_dict, pupdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.pupdevices.UltrasonicSensor)
STATIC const mp_obj_type_t pupdevices_UltrasonicSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_UltrasonicSensor,
    .make_new = pupdevices_UltrasonicSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_UltrasonicSensor_locals_dict,
};

// Class structure for ForceSensor
typedef struct _pupdevices_ForceSensor_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
    int32_t raw_released;
    int32_t raw_offset;
    int32_t raw_start;
    int32_t raw_end;
} pupdevices_ForceSensor_obj_t;

// pybricks.pupdevices.ForceSensor._raw
STATIC int32_t pupdevices_ForceSensor__raw(pbdevice_t *pbdev) {
    int32_t raw;
    pbdevice_get_values(pbdev, PBIO_IODEV_MODE_PUP_FORCE_SENSOR__FRAW, &raw);
    return raw;
}

// pybricks.pupdevices.ForceSensor._force
STATIC int32_t pupdevices_ForceSensor__force(pupdevices_ForceSensor_obj_t *self) {
    // Get raw sensor value
    int32_t raw = pupdevices_ForceSensor__raw(self->pbdev);

    // Get force in millinewtons
    int32_t force = (10000 * (raw - self->raw_released - self->raw_offset)) / (self->raw_end - self->raw_released);

    // With LEGO scaling, initial section is negative, so mask it and return
    return force < 0 ? 0 : force;
}

// pybricks.pupdevices.ForceSensor._distance
STATIC int32_t pupdevices_ForceSensor__distance(pupdevices_ForceSensor_obj_t *self) {
    int32_t raw = pupdevices_ForceSensor__raw(self->pbdev);

    // Get distance in micrometers
    return (6670 * (raw - self->raw_released)) / (self->raw_end - self->raw_released);
}

// pybricks.pupdevices.ForceSensor.__init__
STATIC mp_obj_t pupdevices_ForceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ForceSensor_obj_t *self = m_new_obj(pupdevices_ForceSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_SPIKE_FORCE_SENSOR);

    // Read scaling factors
    int32_t calib[8];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_FORCE_SENSOR__CALIB, calib);
    self->raw_offset = calib[1];
    self->raw_released = calib[2];
    self->raw_end = calib[6];

    // Do sanity check on values to verify calibration read succeeded
    if (self->raw_released >= self->raw_end) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    // Do one read to verify everything works and put it in the right mode
    pupdevices_ForceSensor__raw(self->pbdev);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ForceSensor.touched
STATIC mp_obj_t pupdevices_ForceSensor_touched(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return true if raw value is just above detectable change, with a small
    // margin to account for small calibration tolerances.
    return mp_obj_new_bool(pupdevices_ForceSensor__raw(self->pbdev) > self->raw_released + 4);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ForceSensor_touched_obj, pupdevices_ForceSensor_touched);

// pybricks.pupdevices.ForceSensor.force
STATIC mp_obj_t pupdevices_ForceSensor_force(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return force in newtons
    return pb_obj_new_fraction(pupdevices_ForceSensor__force(self), 1000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ForceSensor_force_obj, pupdevices_ForceSensor_force);

// pybricks.pupdevices.ForceSensor.distance
STATIC mp_obj_t pupdevices_ForceSensor_distance(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return in millimeters
    return pb_obj_new_fraction(pupdevices_ForceSensor__distance(self), 1000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ForceSensor_distance_obj, pupdevices_ForceSensor_distance);

// pybricks.pupdevices.ForceSensor.pressed
STATIC mp_obj_t pupdevices_ForceSensor_pressed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ForceSensor_obj_t, self,
        PB_ARG_DEFAULT_INT(force, 3));

    // Get force threshold in hundreds of newtons
    int32_t f_arg = pbio_math_mul_i32_fix16(1000, pb_obj_get_fix16(force));

    // Return true if the force is bigger than given threshold
    return mp_obj_new_bool(pupdevices_ForceSensor__force(self) >= f_arg);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ForceSensor_pressed_obj, 1, pupdevices_ForceSensor_pressed);

// dir(pybricks.pupdevices.ForceSensor)
STATIC const mp_rom_map_elem_t pupdevices_ForceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_touched),     MP_ROM_PTR(&pupdevices_ForceSensor_touched_obj)              },
    { MP_ROM_QSTR(MP_QSTR_force),       MP_ROM_PTR(&pupdevices_ForceSensor_force_obj)                },
    { MP_ROM_QSTR(MP_QSTR_pressed),     MP_ROM_PTR(&pupdevices_ForceSensor_pressed_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_ForceSensor_distance_obj)             },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ForceSensor_locals_dict, pupdevices_ForceSensor_locals_dict_table);

// type(pybricks.pupdevices.ForceSensor)
STATIC const mp_obj_type_t pupdevices_ForceSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ForceSensor,
    .make_new = pupdevices_ForceSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_ForceSensor_locals_dict,
};

// Class structure for InfraredSensor
typedef struct _pupdevices_InfraredSensor_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
    int32_t count_offset;
} pupdevices_InfraredSensor_obj_t;

// pybricks.pupdevices.InfraredSensor._raw
STATIC int32_t pupdevices_InfraredSensor__raw(pbdevice_t *pbdev) {
    int32_t raw[3];
    pbdevice_get_values(pbdev, PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL, raw);
    return raw[0];
}

// pybricks.pupdevices.InfraredSensor.__init__
STATIC mp_obj_t pupdevices_InfraredSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_InfraredSensor_obj_t *self = m_new_obj(pupdevices_InfraredSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get iodevice
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_WEDO2_MOTION_SENSOR);

    // Reset sensor counter and get sensor back in sensing mode
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT, &self->count_offset);
    pupdevices_InfraredSensor__raw(self->pbdev);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.InfraredSensor.count
STATIC mp_obj_t pupdevices_InfraredSensor_count(mp_obj_t self_in) {
    pupdevices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t count;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT, &count);
    return mp_obj_new_int(count - self->count_offset);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_InfraredSensor_count_obj, pupdevices_InfraredSensor_count);

// pybricks.pupdevices.InfraredSensor.reflection
STATIC mp_obj_t pupdevices_InfraredSensor_reflection(mp_obj_t self_in) {
    pupdevices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t raw = pupdevices_InfraredSensor__raw(self->pbdev);
    return pb_obj_new_fraction(raw, 5);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_InfraredSensor_reflection_obj, pupdevices_InfraredSensor_reflection);

// pybricks.pupdevices.InfraredSensor.distance
STATIC mp_obj_t pupdevices_InfraredSensor_distance(mp_obj_t self_in) {
    pupdevices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t raw = pupdevices_InfraredSensor__raw(self->pbdev);
    return mp_obj_new_int(1100 / (10 + raw));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_InfraredSensor_distance_obj, pupdevices_InfraredSensor_distance);

// dir(pybricks.pupdevices.InfraredSensor)
STATIC const mp_rom_map_elem_t pupdevices_InfraredSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_count),       MP_ROM_PTR(&pupdevices_InfraredSensor_count_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_InfraredSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_InfraredSensor_distance_obj)             },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_InfraredSensor_locals_dict, pupdevices_InfraredSensor_locals_dict_table);

// type(pybricks.pupdevices.InfraredSensor)
STATIC const mp_obj_type_t pupdevices_InfraredSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_InfraredSensor,
    .make_new = pupdevices_InfraredSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_InfraredSensor_locals_dict,
};

// Class structure for TiltSensor
typedef struct _pupdevices_TiltSensor_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} pupdevices_TiltSensor_obj_t;

// pybricks.pupdevices.TiltSensor.__init__
STATIC mp_obj_t pupdevices_TiltSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_TiltSensor_obj_t *self = m_new_obj(pupdevices_TiltSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get iodevice
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_WEDO2_TILT_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.TiltSensor.tilt
STATIC mp_obj_t pupdevices_TiltSensor_tilt(mp_obj_t self_in) {
    pupdevices_TiltSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t tilt[2];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_WEDO2_TILT_SENSOR__ANGLE, tilt);
    mp_obj_t ret[2];
    ret[0] = mp_obj_new_int(tilt[1]);
    ret[1] = mp_obj_new_int(tilt[0]);
    return mp_obj_new_tuple(2, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_TiltSensor_tilt_obj, pupdevices_TiltSensor_tilt);

// dir(pybricks.pupdevices.TiltSensor)
STATIC const mp_rom_map_elem_t pupdevices_TiltSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_tilt),       MP_ROM_PTR(&pupdevices_TiltSensor_tilt_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_TiltSensor_locals_dict, pupdevices_TiltSensor_locals_dict_table);

// type(pybricks.pupdevices.TiltSensor)
STATIC const mp_obj_type_t pupdevices_TiltSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_TiltSensor,
    .make_new = pupdevices_TiltSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_TiltSensor_locals_dict,
};


// Class structure for Light
typedef struct _pupdevices_Light_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} pupdevices_Light_obj_t;

// pybricks.pupdevices.Light.__init__
STATIC mp_obj_t pupdevices_Light_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_Light_obj_t *self = m_new_obj(pupdevices_Light_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_LPF2_LIGHT);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.Light.on
STATIC mp_obj_t pupdevices_Light_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_Light_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    // Set the brightness
    pbdevice_set_power_supply(self->pbdev, pb_obj_get_int(brightness));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_Light_on_obj, 1, pupdevices_Light_on);

// pybricks.pupdevices.Light.off
STATIC mp_obj_t pupdevices_Light_off(mp_obj_t self_in) {
    pupdevices_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbdevice_set_power_supply(self->pbdev, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_Light_off_obj, pupdevices_Light_off);

// dir(pybricks.pupdevices.Light)
STATIC const mp_rom_map_elem_t pupdevices_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on),      MP_ROM_PTR(&pupdevices_Light_on_obj)           },
    { MP_ROM_QSTR(MP_QSTR_off),     MP_ROM_PTR(&pupdevices_Light_off_obj)          },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_Light_locals_dict, pupdevices_Light_locals_dict_table);

// type(pybricks.pupdevices.Light)
STATIC const mp_obj_type_t pupdevices_Light_type = {
    { &mp_type_type },
    .name = MP_QSTR_Light,
    .make_new = pupdevices_Light_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_Light_locals_dict,
};

// dir(pybricks.pupdevices)
STATIC const mp_rom_map_elem_t pupdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_pupdevices)                  },
    { MP_ROM_QSTR(MP_QSTR_Motor),               MP_ROM_PTR(&motor_Motor_type)                    },
    { MP_ROM_QSTR(MP_QSTR_DCMotor),             MP_ROM_PTR(&motor_DCMotor_type)                  },
    { MP_ROM_QSTR(MP_QSTR_ColorDistanceSensor), MP_ROM_PTR(&pupdevices_ColorDistanceSensor_type) },
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),         MP_ROM_PTR(&pupdevices_ColorSensor_type)         },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor),    MP_ROM_PTR(&pupdevices_UltrasonicSensor_type)    },
    { MP_ROM_QSTR(MP_QSTR_ForceSensor),         MP_ROM_PTR(&pupdevices_ForceSensor_type)         },
    { MP_ROM_QSTR(MP_QSTR_InfraredSensor),      MP_ROM_PTR(&pupdevices_InfraredSensor_type)      },
    { MP_ROM_QSTR(MP_QSTR_TiltSensor),          MP_ROM_PTR(&pupdevices_TiltSensor_type)          },
    { MP_ROM_QSTR(MP_QSTR_Light),               MP_ROM_PTR(&pupdevices_Light_type)               },
};

STATIC MP_DEFINE_CONST_DICT(
    pb_module_pupdevices_globals,
    pupdevices_globals_table);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_pupdevices_globals,
};

#endif // PYBRICKS_PY_PUPDEVICES
