// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pberror.h>
#include <pbio/iodev.h>
#include <pbio/light.h>

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objtype.h"

#include "pbdevice.h"
#include "pbobj.h"
#include "pbkwarg.h"

#include "modmotor.h"
#include "modlight.h"
#include "modparameters.h"

#if PYBRICKS_HUB_EV3

// Generic linear scaling of an analog value between a known min and max to a percentage
STATIC int32_t analog_scale(int32_t mvolts, int32_t mvolts_min, int32_t mvolts_max, bool invert) {
    int32_t scaled = (100*(mvolts-mvolts_min))/(mvolts_max-mvolts_min);
    if (invert) {
        scaled = 100 - scaled;
    }
    return max(0, min(scaled, 100));
}

// pybricks.nxtdevices.UltrasonicSensor class object
typedef struct _nxtdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} nxtdevices_UltrasonicSensor_obj_t;

// pybricks.nxtdevices.UltrasonicSensor.__init__
STATIC mp_obj_t nxtdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    nxtdevices_UltrasonicSensor_obj_t *self = m_new_obj(nxtdevices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.UltrasonicSensor.distance
STATIC mp_obj_t nxtdevices_UltrasonicSensor_distance(mp_obj_t self_in) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t distance;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM, &distance);
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
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_UltrasonicSensor_locals_dict,
};

// pybricks.nxtdevices.TouchSensor class object
typedef struct _nxtdevices_TouchSensor_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} nxtdevices_TouchSensor_obj_t;

// pybricks.nxtdevices.TouchSensor.__init__
STATIC mp_obj_t nxtdevices_TouchSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_TRUE(verify_type)
    );

    nxtdevices_TouchSensor_obj_t *self = m_new_obj(nxtdevices_TouchSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    pbio_iodev_type_id_t id = mp_obj_is_true(verify_type) ? PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR : PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG;

    self->pbdev = pbdevice_get_device(port_num, id);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.TouchSensor.pressed
STATIC mp_obj_t nxtdevices_TouchSensor_pressed(mp_obj_t self_in) {
    nxtdevices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_TOUCH_SENSOR__TOUCH, &analog);
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
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_TouchSensor_locals_dict,
};

// pybricks.nxtdevices.SoundSensor class object
typedef struct _nxtdevices_SoundSensor_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} nxtdevices_SoundSensor_obj_t;

// pybricks.nxtdevices.SoundSensor.__init__
STATIC mp_obj_t nxtdevices_SoundSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    nxtdevices_SoundSensor_obj_t *self = m_new_obj(nxtdevices_SoundSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_ANALOG);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.SoundSensor.db
STATIC mp_obj_t nxtdevices_SoundSensor_db(mp_obj_t self_in) {
    nxtdevices_SoundSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &analog);
    return mp_obj_new_int(analog_scale(analog, 650, 4860, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_SoundSensor_db_obj, nxtdevices_SoundSensor_db);

// pybricks.nxtdevices.SoundSensor.dba
STATIC mp_obj_t nxtdevices_SoundSensor_dba(mp_obj_t self_in) {
    nxtdevices_SoundSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE, &analog);
    return mp_obj_new_int(analog_scale(analog, 650, 4860, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_SoundSensor_dba_obj, nxtdevices_SoundSensor_dba);

// dir(pybricks.ev3devices.SoundSensor)
STATIC const mp_rom_map_elem_t nxtdevices_SoundSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_db),  MP_ROM_PTR(&nxtdevices_SoundSensor_db_obj ) },
    { MP_ROM_QSTR(MP_QSTR_dba), MP_ROM_PTR(&nxtdevices_SoundSensor_dba_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_SoundSensor_locals_dict, nxtdevices_SoundSensor_locals_dict_table);

// type(pybricks.ev3devices.SoundSensor)
STATIC const mp_obj_type_t nxtdevices_SoundSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_SoundSensor,
    .make_new = nxtdevices_SoundSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_SoundSensor_locals_dict,
};

// pybricks.nxtdevices.LightSensor class object
typedef struct _nxtdevices_LightSensor_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} nxtdevices_LightSensor_obj_t;

// pybricks.nxtdevices.LightSensor.__init__
STATIC mp_obj_t nxtdevices_LightSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    nxtdevices_LightSensor_obj_t *self = m_new_obj(nxtdevices_LightSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.LightSensor.ambient
STATIC mp_obj_t nxtdevices_LightSensor_ambient(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__AMBIENT, &analog);
    return mp_obj_new_int(analog_scale(analog, 1906, 4164, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_ambient_obj, nxtdevices_LightSensor_ambient);

// pybricks.nxtdevices.LightSensor.reflection
STATIC mp_obj_t nxtdevices_LightSensor_reflection(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__REFLECT, &analog);
    return mp_obj_new_int(analog_scale(analog, 1906, 3000, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_reflection_obj, nxtdevices_LightSensor_reflection);

// dir(pybricks.ev3devices.LightSensor)
STATIC const mp_rom_map_elem_t nxtdevices_LightSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ambient),  MP_ROM_PTR(&nxtdevices_LightSensor_ambient_obj ) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_LightSensor_reflection_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_LightSensor_locals_dict, nxtdevices_LightSensor_locals_dict_table);

// type(pybricks.ev3devices.LightSensor)
STATIC const mp_obj_type_t nxtdevices_LightSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_LightSensor,
    .make_new = nxtdevices_LightSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_LightSensor_locals_dict,
};

// pybricks.nxtdevices.ColorSensor class object
typedef struct _nxtdevices_ColorSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    pbdevice_t *pbdev;
} nxtdevices_ColorSensor_obj_t;

// pybricks.nxtdevices.ColorSensor.__init__
STATIC mp_obj_t nxtdevices_ColorSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    nxtdevices_ColorSensor_obj_t *self = m_new_obj(nxtdevices_ColorSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pbdevice_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_COLOR_SENSOR);

    // Perform one operation
    int32_t color = 0;
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__LAMP, &color);

    // Create an instance of the Light class
    self->light = light_Light_obj_make_new(self->pbdev, &light_ColorLight_type);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t color_obj(pbio_light_color_t color) {
    switch(color) {
        case PBIO_LIGHT_COLOR_RED:
            return MP_OBJ_FROM_PTR(&pb_const_red);
        case PBIO_LIGHT_COLOR_GREEN:
            return MP_OBJ_FROM_PTR(&pb_const_green);
        case PBIO_LIGHT_COLOR_BLUE:
            return MP_OBJ_FROM_PTR(&pb_const_blue);
        case PBIO_LIGHT_COLOR_YELLOW:
            return MP_OBJ_FROM_PTR(&pb_const_yellow);
        case PBIO_LIGHT_COLOR_BLACK:
            return MP_OBJ_FROM_PTR(&pb_const_black);
        case PBIO_LIGHT_COLOR_WHITE:
            return MP_OBJ_FROM_PTR(&pb_const_white);
        default:
            return mp_const_none;
    }
}

// pybricks.nxtdevices.ColorSensor.all
STATIC mp_obj_t nxtdevices_ColorSensor_all(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t all[5];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    mp_obj_t ret[5];
    for (uint8_t i = 0; i < 4; i++) {
        ret[i] = mp_obj_new_int(all[i]);
    }
    ret[4] = color_obj(all[4]);

    return mp_obj_new_tuple(5, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_all_obj, nxtdevices_ColorSensor_all);

// pybricks.nxtdevices.ColorSensor.reflection
STATIC mp_obj_t nxtdevices_ColorSensor_reflection(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t all[5];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the average of red, green, and blue reflection
    return mp_obj_new_int((all[0]+all[1]+all[2])/3);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_reflection_obj, nxtdevices_ColorSensor_reflection);

// pybricks.nxtdevices.ColorSensor.ambient
STATIC mp_obj_t nxtdevices_ColorSensor_ambient(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t all[5];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the ambient light
    return mp_obj_new_int(all[3]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_ambient_obj, nxtdevices_ColorSensor_ambient);

// pybricks.nxtdevices.ColorSensor.color
STATIC mp_obj_t nxtdevices_ColorSensor_color(mp_obj_t self_in) {
    nxtdevices_ColorSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t all[5];
    pbdevice_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__MEASURE, all);
    // Return the color ID
    return color_obj(all[4]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_ColorSensor_color_obj, nxtdevices_ColorSensor_color);

// dir(pybricks.nxtdevices.ColorSensor)
STATIC const mp_rom_map_elem_t nxtdevices_ColorSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_all),        MP_ROM_PTR(&nxtdevices_ColorSensor_all_obj)                  },
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
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_ColorSensor_locals_dict,
};

#endif // PYBRICKS_HUB_EV3

// dir(pybricks.nxtdevices)
STATIC const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_nxtdevices)              },
#if PYBRICKS_HUB_NXT
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&motor_Motor_type)                },
#else
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),      MP_ROM_PTR(&nxtdevices_TouchSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_SoundSensor),      MP_ROM_PTR(&nxtdevices_SoundSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_LightSensor),      MP_ROM_PTR(&nxtdevices_LightSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_type)},
    { MP_ROM_QSTR(MP_QSTR_ColorSensor),      MP_ROM_PTR(&nxtdevices_ColorSensor_type)     },
#endif
};

STATIC MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_nxtdevices_globals,
};

