// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for UltrasonicSensor
typedef struct _pupdevices_UltrasonicSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    mp_obj_base_t base;
    mp_obj_t lights;
} pupdevices_UltrasonicSensor_obj_t;

// pybricks.pupdevices.UltrasonicSensor.__init__
static mp_obj_t pupdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_UltrasonicSensor_obj_t *self = mp_obj_malloc(pupdevices_UltrasonicSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR);

    // Create an instance of the LightArray class
    self->lights = common_LightArray_obj_make_new(&self->device_base, PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__LIGHT, 4);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.UltrasonicSensor.distance
static mp_obj_t get_distance(mp_obj_t self_in) {
    int16_t *data = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL);
    return mp_obj_new_int(data[0] < 0 || data[0] >= 2000 ? 2000 : data[0]);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_distance_obj, PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL, get_distance);

// pybricks.pupdevices.UltrasonicSensor.presence
static mp_obj_t get_presence(mp_obj_t self_in) {
    int8_t *data = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__LISTN);
    return mp_obj_new_bool(data[0]);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_presence_obj, PBDRV_LEGODEV_MODE_PUP_ULTRASONIC_SENSOR__LISTN, get_presence);

static const pb_attr_dict_entry_t pupdevices_UltrasonicSensor_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_lights, pupdevices_UltrasonicSensor_obj_t, lights),
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.pupdevices.UltrasonicSensor)
static const mp_rom_map_elem_t pupdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance),     MP_ROM_PTR(&get_distance_obj)              },
    { MP_ROM_QSTR(MP_QSTR_presence),     MP_ROM_PTR(&get_presence_obj)              },
};
static MP_DEFINE_CONST_DICT(pupdevices_UltrasonicSensor_locals_dict, pupdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.pupdevices.UltrasonicSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_UltrasonicSensor,
    MP_QSTR_UltrasonicSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_UltrasonicSensor_make_new,
    attr, pb_attribute_handler,
    protocol, pupdevices_UltrasonicSensor_attr_dict,
    locals_dict, &pupdevices_UltrasonicSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
