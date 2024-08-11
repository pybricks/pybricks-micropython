
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

// pybricks.ev3devices.TouchSensor class object
typedef struct _ev3devices_TouchSensor_obj_t {
    pb_type_device_obj_base_t device_base;
} ev3devices_TouchSensor_obj_t;

// pybricks.ev3devices.TouchSensor.__init__
static mp_obj_t ev3devices_TouchSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    ev3devices_TouchSensor_obj_t *self = mp_obj_malloc(ev3devices_TouchSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_EV3_TOUCH_SENSOR);
    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ev3devices.TouchSensor.pressed
static mp_obj_t ev3devices_TouchSensor_pressed(mp_obj_t self_in) {
    int32_t *analog = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_EV3_TOUCH_SENSOR__TOUCH);
    return mp_obj_new_bool(analog[0] > 250);
}
static MP_DEFINE_CONST_FUN_OBJ_1(ev3devices_TouchSensor_pressed_obj, ev3devices_TouchSensor_pressed);


// dir(pybricks.ev3devices.TouchSensor)
static const mp_rom_map_elem_t ev3devices_TouchSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&ev3devices_TouchSensor_pressed_obj) },
};
static MP_DEFINE_CONST_DICT(ev3devices_TouchSensor_locals_dict, ev3devices_TouchSensor_locals_dict_table);

// type(pybricks.ev3devices.TouchSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_ev3devices_TouchSensor,
    MP_QSTR_TouchSensor,
    MP_TYPE_FLAG_NONE,
    make_new, ev3devices_TouchSensor_make_new,
    locals_dict, &ev3devices_TouchSensor_locals_dict);

#endif // PYBRICKS_PY_EV3DEVICES
