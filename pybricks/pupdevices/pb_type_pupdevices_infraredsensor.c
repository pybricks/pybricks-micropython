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

// Class structure for InfraredSensor
typedef struct _pupdevices_InfraredSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    int32_t count_offset;
} pupdevices_InfraredSensor_obj_t;

// pybricks.pupdevices.InfraredSensor.__init__
static mp_obj_t pupdevices_InfraredSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_InfraredSensor_obj_t *self = mp_obj_malloc(pupdevices_InfraredSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_WEDO2_MOTION_SENSOR);

    // Reset sensor counter and get sensor back in sensing mode
    self->count_offset = *(int32_t *)pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT);
    pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.InfraredSensor.count
static mp_obj_t get_count(mp_obj_t self_in) {
    pupdevices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t *count = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT);
    return mp_obj_new_int(count[0] - self->count_offset);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_count_obj, PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT, get_count);

// pybricks.pupdevices.InfraredSensor.reflection
static mp_obj_t get_reflection(mp_obj_t self_in) {
    int16_t *data = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL);
    return pb_obj_new_fraction(data[0], 5);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_reflection_obj, PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL, get_reflection);

// pybricks.pupdevices.InfraredSensor.distance
static mp_obj_t get_distance(mp_obj_t self_in) {
    int16_t *data = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL);
    return mp_obj_new_int(1100 / (10 + data[0]));
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_distance_obj, PBDRV_LEGODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL, get_distance);

// dir(pybricks.pupdevices.InfraredSensor)
static const mp_rom_map_elem_t pupdevices_InfraredSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_count),       MP_ROM_PTR(&get_count_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&get_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&get_distance_obj)             },
};
static MP_DEFINE_CONST_DICT(pupdevices_InfraredSensor_locals_dict, pupdevices_InfraredSensor_locals_dict_table);

// type(pybricks.pupdevices.InfraredSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_InfraredSensor,
    MP_QSTR_InfraredSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_InfraredSensor_make_new,
    locals_dict, &pupdevices_InfraredSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
