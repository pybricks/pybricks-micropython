// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pberror.h>
#include <pbio/iodev.h>
#include <pbio/ev3device.h>

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objtype.h"

#include "pbobj.h"
#include "pbkwarg.h"

#include "modmotor.h"
#include "modparameters.h"

// pybricks.nxtdevices.UltrasonicSensor class object
typedef struct _nxtdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pbio_ev3iodev_t *iodev;
#else
    pbio_port_t port;
#endif
} nxtdevices_UltrasonicSensor_obj_t;

// pybricks.nxtdevices.UltrasonicSensor.__init__
STATIC mp_obj_t nxtdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    nxtdevices_UltrasonicSensor_obj_t *self = m_new_obj(nxtdevices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR, port_num));
#else
    self->port = port_num;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.UltrasonicSensor.__str__
STATIC void nxtdevices_UltrasonicSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_UltrasonicSensor));

    pbio_port_t port;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    port = self->iodev->port;
#else
    port = self->port;
#endif
    mp_printf(print, " on Port.S%c",  port);
}

// pybricks.nxtdevices.UltrasonicSensor.distance
STATIC mp_obj_t nxtdevices_UltrasonicSensor_distance(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    uint8_t distance;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM, &distance));
#else
    distance = self->port;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return mp_obj_new_int(distance * 10);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(nxtdevices_UltrasonicSensor_distance_obj, 0, nxtdevices_UltrasonicSensor_distance);

// dir(pybricks.nxtdevices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t nxtdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_distance_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_UltrasonicSensor_locals_dict, nxtdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.nxtdevices.UltrasonicSensor)
STATIC const mp_obj_type_t nxtdevices_UltrasonicSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_UltrasonicSensor,
    .print = nxtdevices_UltrasonicSensor_print,
    .make_new = nxtdevices_UltrasonicSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_UltrasonicSensor_locals_dict,
};

// pybricks.nxtdevices.TouchSensor class object
typedef struct _nxtdevices_TouchSensor_obj_t {
    mp_obj_base_t base;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pbio_ev3iodev_t *iodev;
#else
    pbio_port_t port;
#endif
} nxtdevices_TouchSensor_obj_t;

// pybricks.nxtdevices.TouchSensor.__init__
STATIC mp_obj_t nxtdevices_TouchSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_TRUE(verify_type)
    );

    nxtdevices_TouchSensor_obj_t *self = m_new_obj(nxtdevices_TouchSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    if (mp_obj_is_true(verify_type)) {
        // Get the device and assert that it is of the right type
        pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_NXT_TOUCH_SENSOR, port_num));
    }
    else {
        // Set the sensor as a custom NXT Analog Sensor, so the old-style NXT Touch Sensor will work
        pbio_error_t err = PBIO_ERROR_AGAIN;
        while (err == PBIO_ERROR_AGAIN) {
            err = ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG, port_num);
            mp_hal_delay_ms(500);
        }
        pb_assert(err);
    }
#else
    mp_obj_is_true(verify_type);
    self->port = port_num;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.TouchSensor.__str__
STATIC void nxtdevices_TouchSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    nxtdevices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_TouchSensor));
    pbio_port_t port;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    port = self->iodev->port;
#else
    port = self->port;
#endif
    mp_printf(print, " on Port.S%c",  port);
}

// pybricks.nxtdevices.TouchSensor.pressed
STATIC mp_obj_t nxtdevices_TouchSensor_pressed(mp_obj_t self_in) {
    nxtdevices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_EV3_TOUCH_SENSOR__TOUCH, &analog));
#else
    analog = self->port;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
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
    .print = nxtdevices_TouchSensor_print,
    .make_new = nxtdevices_TouchSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_TouchSensor_locals_dict,
};

// pybricks.nxtdevices.SoundSensor class object
typedef struct _nxtdevices_SoundSensor_obj_t {
    mp_obj_base_t base;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pbio_ev3iodev_t *iodev;
#else
    pbio_port_t port;
#endif
} nxtdevices_SoundSensor_obj_t;

// pybricks.nxtdevices.SoundSensor.__init__
STATIC mp_obj_t nxtdevices_SoundSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    nxtdevices_SoundSensor_obj_t *self = m_new_obj(nxtdevices_SoundSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    // Get the device and assert that it is of the right type
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_NXT_ANALOG, port_num));
#else
    self->port = port_num;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.SoundSensor.__str__
STATIC void nxtdevices_SoundSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    nxtdevices_SoundSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_SoundSensor));
    pbio_port_t port;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    port = self->iodev->port;
#else
    port = self->port;
#endif
    mp_printf(print, " on Port.S%c",  port);
}

#define SOUND_VOLT_MIN (650)
#define SOUND_VOLT_MAX (4860)

STATIC int32_t analog_sound(int32_t mvolts) {
    int32_t sound = 100-(100*(mvolts-SOUND_VOLT_MIN))/(SOUND_VOLT_MAX-SOUND_VOLT_MIN);
    return max(-100, min(sound, 100));
}

// pybricks.nxtdevices.SoundSensor.db
STATIC mp_obj_t nxtdevices_SoundSensor_db(mp_obj_t self_in) {
    nxtdevices_SoundSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &analog));
#else
    analog = self->port;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return mp_obj_new_int(analog_sound(analog));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_SoundSensor_db_obj, nxtdevices_SoundSensor_db);

// pybricks.nxtdevices.SoundSensor.dba
STATIC mp_obj_t nxtdevices_SoundSensor_dba(mp_obj_t self_in) {
    nxtdevices_SoundSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE, &analog));
#else
    analog = self->port;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return mp_obj_new_int(analog_sound(analog));
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
    .print = nxtdevices_SoundSensor_print,
    .make_new = nxtdevices_SoundSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_SoundSensor_locals_dict,
};

// pybricks.nxtdevices.LightSensor class object
typedef struct _nxtdevices_LightSensor_obj_t {
    mp_obj_base_t base;
    bool compensate_ambient;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pbio_ev3iodev_t *iodev;
#else
    pbio_port_t port;
#endif
} nxtdevices_LightSensor_obj_t;

// pybricks.nxtdevices.LightSensor.__init__
STATIC mp_obj_t nxtdevices_LightSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_FALSE(compensation)
    );

    nxtdevices_LightSensor_obj_t *self = m_new_obj(nxtdevices_LightSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    self->compensate_ambient = mp_obj_is_true(compensation);

#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    // Get the device and assert that it is of the right type
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR, port_num));
#else
    self->port = port_num;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.LightSensor.__str__
STATIC void nxtdevices_LightSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_LightSensor));
    pbio_port_t port;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    port = self->iodev->port;
#else
    port = self->port;
#endif
    mp_printf(print, " on Port.S%c",  port);
}

#define LIGHT_VOLT_MIN (1906)
#define LIGHT_VOLT_MAX (4164)

STATIC int32_t analog_light(int32_t mvolts) {
    int32_t light = 100-(100*(mvolts-LIGHT_VOLT_MIN))/(LIGHT_VOLT_MAX-LIGHT_VOLT_MIN);
    return max(-100, min(light, 100));
}

// pybricks.nxtdevices.LightSensor.ambient
STATIC mp_obj_t nxtdevices_LightSensor_ambient(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__AMBIENT, &analog));
#else
    analog = self->port;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return mp_obj_new_int(analog_light(analog));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_ambient_obj, nxtdevices_LightSensor_ambient);

// pybricks.nxtdevices.LightSensor.reflection
STATIC mp_obj_t nxtdevices_LightSensor_reflection(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    int32_t ambient = 0;
    int32_t reflection;
#ifdef PBDRV_CONFIG_HUB_EV3BRICK
    if (self->compensate_ambient) {
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__AMBIENT, &analog));
        mp_hal_delay_ms(30);
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__AMBIENT, &analog));
        ambient = analog_light(analog);
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__REFLECT, &analog));
        mp_hal_delay_ms(30);
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__REFLECT, &analog));
        reflection = analog_light(analog);
    }
    else {
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_COLOR_SENSOR__REFLECT, &analog));
        reflection = analog_light(analog);
        ambient = 0;
    }
#else
    analog = 0;
    reflection = analog;
    ambient = self->port;
    pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
#endif
    return mp_obj_new_int(max(reflection-ambient, 0));
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
    .print = nxtdevices_LightSensor_print,
    .make_new = nxtdevices_LightSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_LightSensor_locals_dict,
};

// dir(pybricks.nxtdevices)
STATIC const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_nxtdevices)              },
#ifdef PBDRV_CONFIG_HUB_NXTBRICK
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&motor_Motor_type)                },
#endif
    { MP_ROM_QSTR(MP_QSTR_TouchSensor),      MP_ROM_PTR(&nxtdevices_TouchSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_SoundSensor),      MP_ROM_PTR(&nxtdevices_SoundSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_LightSensor),      MP_ROM_PTR(&nxtdevices_LightSensor_type)     },
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_type)},
};

STATIC MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_nxtdevices_globals,
};

