// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

// Class structure for InfraredSensor
typedef struct _pupdevices_InfraredSensor_obj_t {
    mp_obj_base_t base;
    pbio_iodev_t *iodev;
    int32_t count_offset;
} pupdevices_InfraredSensor_obj_t;

// pybricks.pupdevices.InfraredSensor._raw
STATIC int32_t pupdevices_InfraredSensor__raw(pbio_iodev_t *iodev) {
    int32_t *raw = pb_pup_device_get_data(iodev, PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__CAL);
    return raw[0];
}

// pybricks.pupdevices.InfraredSensor._count
STATIC int32_t pupdevices_InfraredSensor__count(pbio_iodev_t *iodev) {
    int32_t *count = pb_pup_device_get_data(iodev, PBIO_IODEV_MODE_PUP_WEDO2_MOTION_SENSOR__COUNT);
    return *count;
}

// pybricks.pupdevices.InfraredSensor.__init__
STATIC mp_obj_t pupdevices_InfraredSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_InfraredSensor_obj_t *self = mp_obj_malloc(pupdevices_InfraredSensor_obj_t, type);

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevice
    self->iodev = pb_pup_device_get_device(port, PBIO_IODEV_TYPE_ID_WEDO2_MOTION_SENSOR);

    // Reset sensor counter and get sensor back in sensing mode
    self->count_offset = pupdevices_InfraredSensor__count(self->iodev);
    pupdevices_InfraredSensor__raw(self->iodev);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.InfraredSensor.count
STATIC mp_obj_t pupdevices_InfraredSensor_count(mp_obj_t self_in) {
    pupdevices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(pupdevices_InfraredSensor__count(self->iodev) - self->count_offset);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_InfraredSensor_count_obj, pupdevices_InfraredSensor_count);

// pybricks.pupdevices.InfraredSensor.reflection
STATIC mp_obj_t pupdevices_InfraredSensor_reflection(mp_obj_t self_in) {
    pupdevices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t raw = pupdevices_InfraredSensor__raw(self->iodev);
    return pb_obj_new_fraction(raw, 5);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_InfraredSensor_reflection_obj, pupdevices_InfraredSensor_reflection);

// pybricks.pupdevices.InfraredSensor.distance
STATIC mp_obj_t pupdevices_InfraredSensor_distance(mp_obj_t self_in) {
    pupdevices_InfraredSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t raw = pupdevices_InfraredSensor__raw(self->iodev);
    return mp_obj_new_int(1100 / (10 + raw));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_InfraredSensor_distance_obj, pupdevices_InfraredSensor_distance);

// dir(pybricks.pupdevices.InfraredSensor)
STATIC const mp_rom_map_elem_t pupdevices_InfraredSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_count),       MP_ROM_PTR(&pupdevices_InfraredSensor_count_obj)                },
    { MP_ROM_QSTR(MP_QSTR_reflection),  MP_ROM_PTR(&pupdevices_InfraredSensor_reflection_obj)           },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_InfraredSensor_distance_obj)             },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_InfraredSensor_locals_dict, pupdevices_InfraredSensor_locals_dict_table);

// type(pybricks.pupdevices.InfraredSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_InfraredSensor,
    MP_QSTR_InfraredSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_InfraredSensor_make_new,
    locals_dict, &pupdevices_InfraredSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
