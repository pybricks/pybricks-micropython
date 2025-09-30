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

// pybricks.ev3devices.GyroSensor class object
typedef struct _ev3devices_GyroSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    pbio_direction_t direction;
    mp_int_t offset;
    pbio_os_timer_t timer;
} ev3devices_GyroSensor_obj_t;

static void ev3devices_GyroSensor_reset_angle_offset(mp_obj_t self_in, pbio_direction_t direction, mp_int_t new_angle) {
    // Read raw sensor values
    int16_t *data = pb_type_device_get_data(self_in, LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A);

    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Get new offset using arguments and raw values
    if (direction == PBIO_DIRECTION_CLOCKWISE) {
        self->offset = data[0] - new_angle;
    } else {
        self->offset = -data[0] - new_angle;
    }
}

// pybricks.ev3devices.GyroSensor.__init__
static mp_obj_t ev3devices_GyroSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_OBJ(direction, pb_Direction_CLOCKWISE_obj));

    ev3devices_GyroSensor_obj_t *self = mp_obj_malloc(ev3devices_GyroSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, LEGO_DEVICE_TYPE_ID_EV3_GYRO_SENSOR);

    // Make sure device is in expected mode.
    pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A);

    self->direction = pb_type_enum_get_value(direction_in, &pb_enum_type_Direction);

    // Read initial angle as offset.
    ev3devices_GyroSensor_reset_angle_offset(MP_OBJ_FROM_PTR(self), self->direction, 0);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.GyroSensor.speed
static mp_obj_t ev3devices_GyroSensor_speed(mp_obj_t self_in) {
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t *data = pb_type_device_get_data(self_in, LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A);

    if (self->direction == PBIO_DIRECTION_CLOCKWISE) {
        return mp_obj_new_int(data[1]);
    } else {
        return mp_obj_new_int(-data[1]);
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_GyroSensor_speed_obj, ev3devices_GyroSensor_speed);

// pybricks.ev3devices.GyroSensor.angle
static mp_obj_t ev3devices_GyroSensor_angle(mp_obj_t self_in) {
    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int16_t *data = pb_type_device_get_data(self_in, LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A);

    if (self->direction == PBIO_DIRECTION_CLOCKWISE) {
        return mp_obj_new_int(data[0] - self->offset);
    } else {
        return mp_obj_new_int(-data[0] - self->offset);
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_GyroSensor_angle_obj, ev3devices_GyroSensor_angle);

// pybricks.ev3devices.GyroSensor.reset_angle
static mp_obj_t ev3devices_GyroSensor_reset_angle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3devices_GyroSensor_obj_t, self,
        PB_ARG_REQUIRED(angle));

    ev3devices_GyroSensor_reset_angle_offset(MP_OBJ_FROM_PTR(self), self->direction, pb_obj_get_int(angle_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ev3devices_GyroSensor_reset_angle_obj, 1, ev3devices_GyroSensor_reset_angle);

static pbio_error_t calibration_thread(pbio_os_state_t *state, mp_obj_t parent_obj) {

    ev3devices_GyroSensor_obj_t *self = MP_OBJ_TO_PTR(parent_obj);
    pbio_port_lump_dev_t *lump = self->device_base.lump_dev;

    pbio_error_t err;
    pbio_os_state_t unused;

    PBIO_OS_ASYNC_BEGIN(state);

    // Kick the LUMP thread to stop.
    err = pbio_port_lump_request_reset(lump);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Port will go through the DCM now. This is a brief, fixed duration where
    // we can't get the state.
    PBIO_OS_AWAIT_MS(state, &self->timer, 300);

    // The sensor initially comes back in mode 0, so this waits for synchronization.
    PBIO_OS_AWAIT(state, &unused, pbio_port_lump_is_ready(lump));

    // The gyro sensor isn't too happy to get a mode change right away, so
    // wait briefly.
    PBIO_OS_AWAIT_MS(state, &self->timer, 100);

    // Now change to gyro and angle mode and wait until ready.
    err = pbio_port_lump_set_mode(lump, LEGO_DEVICE_MODE_EV3_GYRO_SENSOR__G_A);
    PBIO_OS_AWAIT(state, &unused, pbio_port_lump_is_ready(lump));

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

// pybricks.ev3devices.GyroSensor.calibrate
static mp_obj_t ev3devices_GyroSensor_calibrate(mp_obj_t self_in) {
    pb_type_async_t config = {
        .parent_obj = self_in,
        .iter_once = calibration_thread,
    };
    return pb_type_async_wait_or_await(&config, NULL, false);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_GyroSensor_calibrate_obj, ev3devices_GyroSensor_calibrate);

// dir(pybricks.ev3devices.GyroSensor)
static const mp_rom_map_elem_t ev3devices_GyroSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_angle),       MP_ROM_PTR(&ev3devices_GyroSensor_angle_obj)       },
    { MP_ROM_QSTR(MP_QSTR_calibrate),   MP_ROM_PTR(&ev3devices_GyroSensor_calibrate_obj)   },
    { MP_ROM_QSTR(MP_QSTR_reset_angle), MP_ROM_PTR(&ev3devices_GyroSensor_reset_angle_obj) },
    { MP_ROM_QSTR(MP_QSTR_speed),       MP_ROM_PTR(&ev3devices_GyroSensor_speed_obj)       },
};
static MP_DEFINE_CONST_DICT(ev3devices_GyroSensor_locals_dict, ev3devices_GyroSensor_locals_dict_table);

// type(pybricks.ev3devices.GyroSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_ev3devices_GyroSensor,
    MP_QSTR_GyroSensor,
    MP_TYPE_FLAG_NONE,
    make_new, ev3devices_GyroSensor_make_new,
    locals_dict, &ev3devices_GyroSensor_locals_dict);

#endif // PYBRICKS_PY_EV3DEVICES
