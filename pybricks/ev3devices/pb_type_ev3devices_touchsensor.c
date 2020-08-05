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

// pybricks.ev3devices.TouchSensor class object
typedef struct _ev3devices_TouchSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} ev3devices_TouchSensor_obj_t;

// pybricks.ev3devices.TouchSensor.__init__
STATIC mp_obj_t ev3devices_TouchSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    ev3devices_TouchSensor_obj_t *self = m_new_obj(ev3devices_TouchSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_EV3_TOUCH_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.TouchSensor.pressed
STATIC mp_obj_t ev3devices_TouchSensor_pressed(mp_obj_t self_in) {
    ev3devices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_EV3_TOUCH_SENSOR__TOUCH, &analog);
    return mp_obj_new_bool(analog > 250);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_TouchSensor_pressed_obj, ev3devices_TouchSensor_pressed);


// dir(pybricks.ev3devices.TouchSensor)
STATIC const mp_rom_map_elem_t ev3devices_TouchSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&ev3devices_TouchSensor_pressed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3devices_TouchSensor_locals_dict, ev3devices_TouchSensor_locals_dict_table);

// type(pybricks.ev3devices.TouchSensor)
const mp_obj_type_t pb_type_ev3devices_TouchSensor = {
    { &mp_type_type },
    .name = MP_QSTR_TouchSensor,
    .make_new = ev3devices_TouchSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&ev3devices_TouchSensor_locals_dict,
};

#endif // PYBRICKS_PY_EV3DEVICES
