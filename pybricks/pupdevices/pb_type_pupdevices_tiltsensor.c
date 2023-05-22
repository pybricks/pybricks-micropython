// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

// Class structure for TiltSensor
typedef struct _pupdevices_TiltSensor_obj_t {
    mp_obj_base_t base;
    pbio_iodev_t *iodev;
} pupdevices_TiltSensor_obj_t;

// pybricks.pupdevices.TiltSensor.__init__
STATIC mp_obj_t pupdevices_TiltSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_TiltSensor_obj_t *self = mp_obj_malloc(pupdevices_TiltSensor_obj_t, type);

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevice
    self->iodev = pup_device_get_device(port, PBIO_IODEV_TYPE_ID_WEDO2_TILT_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.TiltSensor.tilt
STATIC mp_obj_t pupdevices_TiltSensor_tilt(mp_obj_t self_in) {
    pupdevices_TiltSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t *tilt;
    pup_device_get_data(self->iodev, PBIO_IODEV_MODE_PUP_WEDO2_TILT_SENSOR__ANGLE, (uint8_t **)&tilt);
    mp_obj_t ret[2];
    ret[0] = mp_obj_new_int(tilt[1]);
    ret[1] = mp_obj_new_int(tilt[0]);
    return mp_obj_new_tuple(2, ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_TiltSensor_tilt_obj, pupdevices_TiltSensor_tilt);

// dir(pybricks.pupdevices.TiltSensor)
STATIC const mp_rom_map_elem_t pupdevices_TiltSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_tilt),       MP_ROM_PTR(&pupdevices_TiltSensor_tilt_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_TiltSensor_locals_dict, pupdevices_TiltSensor_locals_dict_table);

// type(pybricks.pupdevices.TiltSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_TiltSensor,
    MP_QSTR_TiltSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_TiltSensor_make_new,
    locals_dict, &pupdevices_TiltSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
