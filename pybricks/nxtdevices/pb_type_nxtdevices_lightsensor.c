// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES

#include <pbio/int_math.h>

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>

// Generic linear scaling of an analog value between a known min and max to a percentage
int32_t analog_scale(int32_t mvolts, int32_t mvolts_min, int32_t mvolts_max, bool invert) {
    int32_t scaled = (100 * (mvolts - mvolts_min)) / (mvolts_max - mvolts_min);
    if (invert) {
        scaled = 100 - scaled;
    }
    return pbio_int_math_bind(scaled, 0, 100);
}

// pybricks.nxtdevices.LightSensor class object
typedef struct _nxtdevices_LightSensor_obj_t {
    pb_type_device_obj_base_t device_base;
} nxtdevices_LightSensor_obj_t;

// pybricks.nxtdevices.LightSensor.ambient
static mp_obj_t nxtdevices_LightSensor_ambient(mp_obj_t self_in) {
    int32_t *analog = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_NXT_LIGHT_SENSOR__AMBIENT);
    return mp_obj_new_int(analog_scale(analog[0], 1906, 4164, true));
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_ambient_obj, nxtdevices_LightSensor_ambient);

// pybricks.nxtdevices.LightSensor.reflection
static mp_obj_t nxtdevices_LightSensor_reflection(mp_obj_t self_in) {
    int32_t *analog = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_NXT_LIGHT_SENSOR__REFLECT);
    return mp_obj_new_int(analog_scale(analog[0], 1906, 3000, true));
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_reflection_obj, nxtdevices_LightSensor_reflection);

// pybricks.nxtdevices.LightSensor.__init__
static mp_obj_t nxtdevices_LightSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_LightSensor_obj_t *self = mp_obj_malloc(nxtdevices_LightSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_NXT_LIGHT_SENSOR);

    // Read one value to ensure a consistent initial mode
    nxtdevices_LightSensor_reflection(MP_OBJ_FROM_PTR(self));

    return MP_OBJ_FROM_PTR(self);
}

// dir(pybricks.ev3devices.LightSensor)
static const mp_rom_map_elem_t nxtdevices_LightSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ambient),  MP_ROM_PTR(&nxtdevices_LightSensor_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_LightSensor_reflection_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_LightSensor_locals_dict, nxtdevices_LightSensor_locals_dict_table);

// type(pybricks.ev3devices.LightSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_LightSensor,
    MP_QSTR_LightSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_LightSensor_make_new,
    locals_dict, &nxtdevices_LightSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
