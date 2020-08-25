// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>

// Generic linear scaling of an analog value between a known min and max to a percentage
int32_t analog_scale(int32_t mvolts, int32_t mvolts_min, int32_t mvolts_max, bool invert) {
    int32_t scaled = (100 * (mvolts - mvolts_min)) / (mvolts_max - mvolts_min);
    if (invert) {
        scaled = 100 - scaled;
    }
    return max(0, min(scaled, 100));
}

// pybricks.nxtdevices.LightSensor class object
typedef struct _nxtdevices_LightSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_LightSensor_obj_t;

// pybricks.nxtdevices.LightSensor.ambient
STATIC mp_obj_t nxtdevices_LightSensor_ambient(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;

    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__AMBIENT, &analog);

    return mp_obj_new_int(analog_scale(analog, 1906, 4164, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_ambient_obj, nxtdevices_LightSensor_ambient);

// pybricks.nxtdevices.LightSensor.reflection
STATIC mp_obj_t nxtdevices_LightSensor_reflection(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t analog;

    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_LIGHT_SENSOR__REFLECT, &analog);

    return mp_obj_new_int(analog_scale(analog, 1906, 3000, true));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_reflection_obj, nxtdevices_LightSensor_reflection);

// pybricks.nxtdevices.LightSensor.__init__
STATIC mp_obj_t nxtdevices_LightSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_LightSensor_obj_t *self = m_new_obj(nxtdevices_LightSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_NXT_LIGHT_SENSOR);

    // Read one value to ensure a consistent initial mode
    nxtdevices_LightSensor_reflection(self);

    return MP_OBJ_FROM_PTR(self);
}

// dir(pybricks.ev3devices.LightSensor)
STATIC const mp_rom_map_elem_t nxtdevices_LightSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ambient),  MP_ROM_PTR(&nxtdevices_LightSensor_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_LightSensor_reflection_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_LightSensor_locals_dict, nxtdevices_LightSensor_locals_dict_table);

// type(pybricks.ev3devices.LightSensor)
const mp_obj_type_t pb_type_nxtdevices_LightSensor = {
    { &mp_type_type },
    .name = MP_QSTR_LightSensor,
    .make_new = nxtdevices_LightSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_LightSensor_locals_dict,
};

#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
