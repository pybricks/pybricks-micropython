// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>

// Class structure for UltrasonicSensor
typedef struct _pupdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t lights;
    pb_device_t *pbdev;
} pupdevices_UltrasonicSensor_obj_t;

// pybricks.pupdevices.UltrasonicSensor.__init__
STATIC mp_obj_t pupdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_UltrasonicSensor_obj_t *self = mp_obj_malloc(pupdevices_UltrasonicSensor_obj_t, type);

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_SPIKE_ULTRASONIC_SENSOR);

    // Create an instance of the LightArray class
    self->lights = common_LightArray_obj_make_new(self->pbdev, PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__LIGHT, 4);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.UltrasonicSensor.distance
STATIC mp_obj_t pupdevices_UltrasonicSensor_distance(mp_obj_t self_in) {
    pupdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t distance;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__DISTL, &distance);
    return mp_obj_new_int(distance < 0 || distance >= 2000 ? 2000 : distance);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_UltrasonicSensor_distance_obj, pupdevices_UltrasonicSensor_distance);

// pybricks.pupdevices.UltrasonicSensor.presence
STATIC mp_obj_t pupdevices_UltrasonicSensor_presence(mp_obj_t self_in) {
    pupdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t presence;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_PUP_ULTRASONIC_SENSOR__LISTN, &presence);
    return mp_obj_new_bool(presence);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_UltrasonicSensor_presence_obj, pupdevices_UltrasonicSensor_presence);

STATIC const pb_attr_dict_entry_t pupdevices_UltrasonicSensor_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_lights, pupdevices_UltrasonicSensor_obj_t, lights),
    PB_ATTR_DICT_SENTINEL
};

// dir(pybricks.pupdevices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t pupdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance),     MP_ROM_PTR(&pupdevices_UltrasonicSensor_distance_obj)              },
    { MP_ROM_QSTR(MP_QSTR_presence),     MP_ROM_PTR(&pupdevices_UltrasonicSensor_presence_obj)              },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_UltrasonicSensor_locals_dict, pupdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.pupdevices.UltrasonicSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_UltrasonicSensor,
    MP_QSTR_UltrasonicSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_UltrasonicSensor_make_new,
    attr, pb_attribute_handler,
    protocol, pupdevices_UltrasonicSensor_attr_dict,
    locals_dict, &pupdevices_UltrasonicSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
