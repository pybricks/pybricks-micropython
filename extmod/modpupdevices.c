// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbdrv/ioport.h>
#include <pbio/iodev.h>
#include <pbio/light.h>

#include "py/runtime.h"

#include "modcommon.h"
#include "modmotor.h"
#include "pberror.h"
#include "pbiodevice.h"

// Class structure for ColorAndDistSensor
typedef struct _pupdevices_ColorAndDistSensor_obj_t {
    mp_obj_base_t base;
    pbio_iodev_t *iodev;
} pupdevices_ColorAndDistSensor_obj_t;

STATIC mp_obj_t pupdevices_ColorAndDistSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    pupdevices_ColorAndDistSensor_obj_t *self = m_new_obj(pupdevices_ColorAndDistSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    pbio_port_t port = mp_obj_get_int(args[0]);

    pb_assert(pbdrv_ioport_get_iodev(port, &self->iodev));
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    pb_assert(pb_iodevice_set_mode(self->iodev, 8));
    return MP_OBJ_FROM_PTR(self);
}

STATIC uint8_t pupdevices_ColorAndDistSensor_combined_mode(pbio_iodev_t *iodev, uint8_t idx) {
    pb_assert(pb_iodevice_set_mode(iodev, 8));
    uint8_t *data;
    pb_assert(pbio_iodev_get_raw_values(iodev, &data));
    return data[idx];
}

STATIC void pupdevices_ColorAndDistSensor_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    mp_printf(print, qstr_str(MP_QSTR_ColorDistanceSensor));
    mp_printf(print, " on Port.%c",  self->iodev->port);
}

STATIC mp_obj_t pupdevices_ColorAndDistSensor_color(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);

    switch(pupdevices_ColorAndDistSensor_combined_mode(self->iodev, 0)) {
        case 0:
            return mp_obj_new_int(PBIO_LIGHT_COLOR_BLACK);
        case 3:
            return mp_obj_new_int(PBIO_LIGHT_COLOR_BLUE);
        case 5:
            return mp_obj_new_int(PBIO_LIGHT_COLOR_GREEN);
        case 7:
            return mp_obj_new_int(PBIO_LIGHT_COLOR_YELLOW);
        case 8:
            return mp_obj_new_int(PBIO_LIGHT_COLOR_ORANGE);
        case 9:
            return mp_obj_new_int(PBIO_LIGHT_COLOR_RED);
        case 10:
            return mp_obj_new_int(PBIO_LIGHT_COLOR_WHITE);
        default:
            return mp_const_none;
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
    pb_assert(pb_iodevice_set_mode(self->iodev, 4));
    return pb_iodevice_get_values(self->iodev);
}
MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ColorAndDistSensor_ambient_obj, pupdevices_ColorAndDistSensor_ambient);

STATIC mp_obj_t pupdevices_ColorAndDistSensor_rgb(mp_obj_t self_in) {
    pupdevices_ColorAndDistSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_iodevice_assert_type_id(self->iodev, PBIO_IODEV_TYPE_ID_COLOR_DIST_SENSOR);
    pb_assert(pb_iodevice_set_mode(self->iodev, 6));
    uint8_t *data;
    pb_assert(pbio_iodev_get_raw_values(self->iodev, &data));
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
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ColorAndDistSensor_locals_dict, pupdevices_ColorAndDistSensor_locals_dict_table);

STATIC const mp_obj_type_t pupdevices_ColorAndDistSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorDistanceSensor,
    .print = pupdevices_ColorAndDistSensor_print,
    .make_new = pupdevices_ColorAndDistSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&pupdevices_ColorAndDistSensor_locals_dict,
};

/*
pupdevices module table
*/
STATIC const mp_rom_map_elem_t pupdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_devices) },
#if PYBRICKS_PY_MOTOR
    { MP_ROM_QSTR(MP_QSTR_Motor),               MP_ROM_PTR(&motor_Motor_type)   },
#endif //PYBRICKS_PY_MOTOR
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
