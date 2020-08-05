// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES

#include "py/mphal.h"
#include "py/runtime.h"

#include "util_pb/pb_device.h"
#include "util_mp/pb_obj_helper.h"
#include "util_mp/pb_kwarg_helper.h"

#include "py/objtype.h"

#include <pbio/iodev.h>
#include <pbio/button.h>

#include "common/common.h"
#include "common/common_motors.h"
#include "ev3devices/ev3devices.h"

#include "parameters/parameters.h"
#include "util_pb/pb_error.h"

// pybricks.ev3devices.UltrasonicSensor class object
typedef struct _ev3devices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} ev3devices_UltrasonicSensor_obj_t;


// pybricks.ev3devices.UltrasonicSensor.__init__
STATIC mp_obj_t ev3devices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    ev3devices_UltrasonicSensor_obj_t *self = m_new_obj(ev3devices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_EV3_ULTRASONIC_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.UltrasonicSensor.distance
STATIC mp_obj_t ev3devices_UltrasonicSensor_distance(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_UltrasonicSensor_obj_t, self,
        PB_ARG_DEFAULT_FALSE(silent));

    int32_t distance;
    if (mp_obj_is_true(silent)) {
        pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__SI_CM, &distance);
    } else {
        pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__DIST_CM, &distance);
    }
    return mp_obj_new_int(distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_UltrasonicSensor_distance_obj, 1, ev3devices_UltrasonicSensor_distance);

// pybricks.ev3devices.UltrasonicSensor.presence
STATIC mp_obj_t ev3devices_UltrasonicSensor_presence(mp_obj_t self_in) {
    ev3devices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t presence;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_ULTRASONIC_SENSOR__LISTEN, &presence);
    return mp_obj_new_bool(presence);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_UltrasonicSensor_presence_obj, ev3devices_UltrasonicSensor_presence);

// dir(pybricks.ev3devices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t ev3devices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&ev3devices_UltrasonicSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_presence), MP_ROM_PTR(&ev3devices_UltrasonicSensor_presence_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_UltrasonicSensor_locals_dict, ev3devices_UltrasonicSensor_locals_dict_table);

// type(pybricks.ev3devices.UltrasonicSensor)
const mp_obj_type_t pb_type_ev3devices_UltrasonicSensor = {
    { &mp_type_type },
    .name = MP_QSTR_UltrasonicSensor,
    .make_new = ev3devices_UltrasonicSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&ev3devices_UltrasonicSensor_locals_dict,
};

#endif // PYBRICKS_PY_EV3DEVICES
