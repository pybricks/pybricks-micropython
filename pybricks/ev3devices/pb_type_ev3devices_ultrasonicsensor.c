// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/ev3devices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>

// pybricks.ev3devices.UltrasonicSensor class object
typedef struct _ev3devices_UltrasonicSensor_obj_t {
    pb_type_device_obj_base_t device_base;
} ev3devices_UltrasonicSensor_obj_t;


// pybricks.ev3devices.UltrasonicSensor.__init__
static mp_obj_t ev3devices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    ev3devices_UltrasonicSensor_obj_t *self = mp_obj_malloc(ev3devices_UltrasonicSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.UltrasonicSensor.distance
static mp_obj_t ev3devices_UltrasonicSensor_distance(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_UltrasonicSensor_obj_t, self,
        PB_ARG_DEFAULT_FALSE(silent));

    uint8_t mode = mp_obj_is_true(silent_in) ? PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM : PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM;
    int16_t *distance = pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), mode);
    return mp_obj_new_int(distance[0]);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_UltrasonicSensor_distance_obj, 1, ev3devices_UltrasonicSensor_distance);

// pybricks.ev3devices.UltrasonicSensor.presence
static mp_obj_t ev3devices_UltrasonicSensor_presence(mp_obj_t self_in) {
    int8_t *presence = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_EV3_ULTRASONIC_SENSOR__LISTEN);
    return mp_obj_new_bool(presence[0]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_UltrasonicSensor_presence_obj, ev3devices_UltrasonicSensor_presence);

// dir(pybricks.ev3devices.UltrasonicSensor)
static const mp_rom_map_elem_t ev3devices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&ev3devices_UltrasonicSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_presence), MP_ROM_PTR(&ev3devices_UltrasonicSensor_presence_obj) },
};
static MP_DEFINE_CONST_DICT(ev3devices_UltrasonicSensor_locals_dict, ev3devices_UltrasonicSensor_locals_dict_table);

// type(pybricks.ev3devices.UltrasonicSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_ev3devices_UltrasonicSensor,
    MP_QSTR_UltrasonicSensor,
    MP_TYPE_FLAG_NONE,
    make_new, ev3devices_UltrasonicSensor_make_new,
    locals_dict, &ev3devices_UltrasonicSensor_locals_dict);

#endif // PYBRICKS_PY_EV3DEVICES
