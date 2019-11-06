// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/light.h>
#include <pbio/button.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "modlight.h"
#include "pbiodevice.h"

#include "modparameters.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.builtins.Light class object
typedef struct _light_Light_obj_t {
    mp_obj_base_t base;
    pbio_lightdev_t dev;
} light_Light_obj_t;

// pybricks.builtins.Light.on
STATIC mp_obj_t light_Light_on(mp_obj_t self_in) {
    light_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Turn the light on, using the command specific to the device.
#if PYBRICKS_PY_PUPDEVICES
    if (self->dev.id == PBIO_IODEV_TYPE_ID_LPF2_LIGHT) {
        // TODO
        return mp_const_none;
    }
#endif
#if PYBRICKS_PY_EV3DEVICES
    if (self->dev.id == PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR) {
        int16_t unused;
        pb_assert(ev3device_get_values_at_mode(self->dev.ev3iodev,
                                               PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM,
                                               &unused));
        return mp_const_none;
    }
#endif
    pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(light_Light_on_obj, light_Light_on);

// pybricks.builtins.ColorLight.on
STATIC mp_obj_t light_ColorLight_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse arguments
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_INT(brightness, 100)
    );
    light_Light_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (color == mp_const_none) {
        color = MP_OBJ_FROM_PTR(&pb_const_black);
    }

    pbio_light_color_t color_id = enum_get_value_maybe(color, &pb_enum_type_Color);

    mp_int_t bright = pb_obj_get_int(brightness);
    bright = bright < 0 ? 0 : bright > 100 ? 100: bright;

    // TODO: Brightness control is not yet implemented
    if (bright != 100) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    // Turn the light on, using the command specific to the device.
#if PYBRICKS_PY_EV3DEVICES
    if (self->dev.id == PBIO_IODEV_TYPE_ID_EV3_COLOR_SENSOR) {
        // TODO
        return mp_const_none;
    }
    else if (self->dev.id == PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR) {
        pb_assert(ev3device_get_values_at_mode(self->dev.ev3iodev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP, &color_id));
        return mp_const_none;
    }
#endif
#if PYBRICKS_PY_PUPDEVICES
    if (self->dev.id == PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR) {
        uint8_t mode;
        switch (color_id) {
            case PBIO_LIGHT_COLOR_GREEN:
                mode = 1;
                break;
            case PBIO_LIGHT_COLOR_RED:
                mode = 3;
                break;
            case PBIO_LIGHT_COLOR_BLUE:
                mode = 4;
                break;
            default:
                mode = 7;
                break;
        }
        pb_iodevice_set_mode(self->dev.pupiodev, mode);
        uint8_t *data;
        pb_assert(pbio_iodev_get_data(self->dev.pupiodev, &data));
        return mp_const_none;
    }
#endif
    // No external device, so assume command is for the internal light
    pb_assert(pbio_light_on(PBIO_PORT_SELF, color_id));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(light_ColorLight_on_obj, 0, light_ColorLight_on);

// pybricks.builtins.Light.off
// pybricks.builtins.LightArray.off
// pybricks.builtins.ColorLight.off
STATIC mp_obj_t light_Light_off(mp_obj_t self_in) {
    light_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Turn the light off, using the command specific to the device. 

#if PYBRICKS_PY_PUPDEVICES
    if (self->dev.id == PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR) {
        pb_iodevice_set_mode(self->dev.pupiodev, 7);
        uint8_t *data;
        pb_assert(pbio_iodev_get_data(self->dev.pupiodev, &data));
        return mp_const_none;
    }
#endif
#if PYBRICKS_PY_EV3DEVICES
    if (self->dev.id == PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR) {
        int16_t unused;
        pb_assert(ev3device_get_values_at_mode(self->dev.ev3iodev,
                                               PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM,
                                               &unused));
        return mp_const_none;
    }
#endif

    // No external device, so assume command is for the internal light
    pb_assert(pbio_light_off(PBIO_PORT_SELF));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(light_Light_off_obj, light_Light_off);

// dir(pybricks.builtins.Light)
STATIC const mp_rom_map_elem_t light_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&light_Light_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&light_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(light_Light_locals_dict, light_Light_locals_dict_table);

// type(pybricks.builtins.Light)
const mp_obj_type_t light_Light_type = {
    { &mp_type_type },
    .name = MP_QSTR_Light,
    .locals_dict = (mp_obj_dict_t*)&light_Light_locals_dict,
};

// dir(pybricks.builtins.ColorLight)
STATIC const mp_rom_map_elem_t light_ColorLight_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&light_ColorLight_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&light_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(light_ColorLight_locals_dict, light_ColorLight_locals_dict_table);

// type(pybricks.builtins.ColorLight)
const mp_obj_type_t light_ColorLight_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorLight,
    .locals_dict = (mp_obj_dict_t*)&light_ColorLight_locals_dict,
};

mp_obj_t light_Light_obj_make_new(pbio_lightdev_t dev, const mp_obj_type_t *type) {
    // Create new light instance
    light_Light_obj_t *light = m_new_obj(light_Light_obj_t);
    // Set type and iodev
    light->base.type = type;
    light->dev = dev;
    return light;
}
