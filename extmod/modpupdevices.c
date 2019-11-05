// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>
#include <pbio/light.h>

#include "py/runtime.h"

#include "pberror.h"
#include "pbkwarg.h"

#include "modlight.h"
#include "modparameters.h"
#include "modmotor.h"
#include "pbiodevice.h"

// Class structure for ColorAndDistSensor
typedef struct _pupdevices_ColorAndDistSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    pbio_iodev_t *iodev;
} pupdevices_ColorAndDistSensor_obj_t;

// pybricks.ev3devices.pupdevices_ColorAndDistSensor.__init__
STATIC mp_obj_t pupdevices_ColorAndDistSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    pupdevices_ColorAndDistSensor_obj_t *self = m_new_obj(pupdevices_ColorAndDistSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
    pb_assert(pbdrv_ioport_get_iodev(port_num, &self->iodev));
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    pb_iodevice_set_mode(self->iodev, 8);

    // Create an instance of the Light class
    pbio_lightdev_t dev = {
        .id = self->iodev->info->type_id,
        .pupiodev = self->iodev,
    };
    self->light = light_Light_obj_make_new(dev, &light_ColorLight_type);

    return MP_OBJ_FROM_PTR(self);
}

STATIC uint8_t pupdevices_ColorAndDistSensor_combined_mode(pbio_iodev_t *iodev, uint8_t idx) {
    pb_iodevice_set_mode(iodev, 8);
    uint8_t *data;
    pb_assert(pbio_iodev_get_data(iodev, &data));
    return data[idx];
}

STATIC mp_obj_t pupdevices_ColorAndDistSensor_color(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);

    switch(pupdevices_ColorAndDistSensor_combined_mode(self->iodev, 0)) {
        case 1: return MP_OBJ_FROM_PTR(&pb_const_black);
        case 3: return MP_OBJ_FROM_PTR(&pb_const_blue);
        case 5: return MP_OBJ_FROM_PTR(&pb_const_green);
        case 7: return MP_OBJ_FROM_PTR(&pb_const_yellow);
        case 8: return MP_OBJ_FROM_PTR(&pb_const_orange);
        case 9: return MP_OBJ_FROM_PTR(&pb_const_red);
        case 10: return MP_OBJ_FROM_PTR(&pb_const_white);
        default: return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_color_obj, pupdevices_ColorAndDistSensor_color);

STATIC mp_obj_t pupdevices_ColorAndDistSensor_distance(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    return mp_obj_new_int(pupdevices_ColorAndDistSensor_combined_mode(self->iodev, 1));
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_distance_obj, pupdevices_ColorAndDistSensor_distance);

STATIC mp_obj_t pupdevices_ColorAndDistSensor_reflection(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    return mp_obj_new_int(pupdevices_ColorAndDistSensor_combined_mode(self->iodev, 3));
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_reflection_obj, pupdevices_ColorAndDistSensor_reflection);

STATIC mp_obj_t pupdevices_ColorAndDistSensor_ambient(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    pb_iodevice_set_mode(self->iodev, 4);
    return pb_iodevice_get_values(self->iodev);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_ambient_obj, pupdevices_ColorAndDistSensor_ambient);

STATIC mp_obj_t pupdevices_ColorAndDistSensor_rgb(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    pb_iodevice_set_mode(self->iodev, 6);
    uint8_t *data;
    pb_assert(pbio_iodev_get_data(self->iodev, &data));
    mp_obj_t rgb[3];
    for (uint8_t col = 0; col < 3; col++) {
        int16_t intensity = ((*(int16_t *)(data + col * 2))*10)/44;
        rgb[col] = mp_obj_new_int(intensity < 100 ? intensity : 100);
    }
    return mp_obj_new_tuple(3, rgb);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_rgb_obj, pupdevices_ColorAndDistSensor_rgb);

/*
ColorAndDistSensor class tables
*/
STATIC const mp_rom_map_elem_t pupdevices_ColorAndDistSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_color), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_reflection_obj) },
    { MP_ROM_QSTR(MP_QSTR_ambient), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_rgb), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_rgb_obj) },
    { MP_ROM_QSTR(MP_QSTR_light), MP_ROM_ATTRIBUTE_OFFSET(pupdevices_ColorAndDistSensor_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorAndDistSensor_locals_dict, pupdevices_ColorAndDistSensor_locals_dict_table);

STATIC const mp_obj_type_t pupdevices_ColorAndDistSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorDistanceSensor,
    .make_new = pupdevices_ColorAndDistSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&pupdevices_ColorAndDistSensor_locals_dict,
};

/*
pupdevices module table
*/
STATIC const mp_rom_map_elem_t pupdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_devices) },
    { MP_ROM_QSTR(MP_QSTR_Motor),               MP_ROM_PTR(&motor_Motor_type)   },
    { MP_ROM_QSTR(MP_QSTR_ColorDistanceSensor), MP_ROM_PTR(&pupdevices_ColorAndDistSensor_type) },
};

STATIC MP_DEFINE_CONST_DICT (
    pb_module_pupdevices_globals,
    pupdevices_globals_table
);

const mp_obj_module_t pb_module_pupdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_pupdevices_globals,
};

#endif // PYBRICKS_PY_PUPDEVICES
