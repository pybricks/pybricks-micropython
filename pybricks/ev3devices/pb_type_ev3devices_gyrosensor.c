// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/ev3devices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>

// pybricks.ev3devices.GyroSensor class object
typedef struct _ev3devices_GyroSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t port; // FIXME: Shouldn't be here
    pb_device_t *pbdev;
    pbio_direction_t direction;
    mp_int_t offset;
} ev3devices_GyroSensor_obj_t;

// pybricks.ev3devices.GyroSensor (internal) Get new offset  for new reset angle
STATIC mp_int_t ev3devices_GyroSensor_get_angle_offset(pb_device_t *pbdev, pbio_direction_t direction, mp_int_t new_angle) {
    // Read raw sensor values
    int32_t raw_angle;
    pb_device_get_values(pbdev, PBIO_IODEV_MODE_EV3_GYRO_SENSOR__ANG, &raw_angle);

    // Get new offset using arguments and raw values
    if (direction == PBIO_DIRECTION_CLOCKWISE) {
        return raw_angle - new_angle;
    } else {
        return -raw_angle - new_angle;
    }
}

// pybricks.ev3devices.GyroSensor.__init__
STATIC mp_obj_t ev3devices_GyroSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_OBJ(direction, pb_Direction_CLOCKWISE_obj));

    ev3devices_GyroSensor_obj_t *self = m_new_obj(ev3devices_GyroSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->direction = pb_type_enum_get_value(direction_in, &pb_enum_type_Direction);

    self->port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(self->port, PBIO_IODEV_TYPE_ID_EV3_GYRO_SENSOR);

    self->offset = ev3devices_GyroSensor_get_angle_offset(self->pbdev, self->direction, 0);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.GyroSensor.speed
STATIC mp_obj_t ev3devices_GyroSensor_speed(mp_obj_t self_in) {
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t raw_speed;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_GYRO_SENSOR__RATE, &raw_speed);

    // changing modes resets angle to 0
    self->offset = 0;

    if (self->direction == PBIO_DIRECTION_CLOCKWISE) {
        return mp_obj_new_int(raw_speed);
    } else {
        return mp_obj_new_int(-raw_speed);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_GyroSensor_speed_obj, ev3devices_GyroSensor_speed);

// pybricks.ev3devices.GyroSensor.angle
STATIC mp_obj_t ev3devices_GyroSensor_angle(mp_obj_t self_in) {
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t raw_angle;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_GYRO_SENSOR__ANG, &raw_angle);

    if (self->direction == PBIO_DIRECTION_CLOCKWISE) {
        return mp_obj_new_int(raw_angle - self->offset);
    } else {
        return mp_obj_new_int(-raw_angle - self->offset);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_GyroSensor_angle_obj, ev3devices_GyroSensor_angle);

// pybricks.ev3devices.GyroSensor.reset_angle
STATIC mp_obj_t ev3devices_GyroSensor_reset_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_GyroSensor_obj_t, self,
        PB_ARG_REQUIRED(angle));

    self->offset = ev3devices_GyroSensor_get_angle_offset(self->pbdev, self->direction, pb_obj_get_int(angle_in));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_GyroSensor_reset_angle_obj, 1, ev3devices_GyroSensor_reset_angle);

// dir(pybricks.ev3devices.GyroSensor)
STATIC const mp_rom_map_elem_t ev3devices_GyroSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_angle),       MP_ROM_PTR(&ev3devices_GyroSensor_angle_obj)       },
    { MP_ROM_QSTR(MP_QSTR_speed),       MP_ROM_PTR(&ev3devices_GyroSensor_speed_obj)       },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&ev3devices_GyroSensor_reset_angle_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_GyroSensor_locals_dict, ev3devices_GyroSensor_locals_dict_table);

// type(pybricks.ev3devices.GyroSensor)
const mp_obj_type_t pb_type_ev3devices_GyroSensor = {
    { &mp_type_type },
    .name = MP_QSTR_GyroSensor,
    .make_new = ev3devices_GyroSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&ev3devices_GyroSensor_locals_dict,
};

#endif // PYBRICKS_PY_EV3DEVICES
