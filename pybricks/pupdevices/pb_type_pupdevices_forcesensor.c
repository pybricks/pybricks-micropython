// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbio/int_math.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for ForceSensor
typedef struct _pupdevices_ForceSensor_obj_t {
    mp_obj_base_t base;
    pbio_iodev_t *iodev;
    int32_t raw_released;
    int32_t raw_offset;
    int32_t raw_start;
    int32_t raw_end;
} pupdevices_ForceSensor_obj_t;

// pybricks.pupdevices.ForceSensor._raw
STATIC int32_t pupdevices_ForceSensor__raw(pbio_iodev_t *iodev) {
    int16_t *raw;
    pb_pup_device_get_data(iodev, PBIO_IODEV_MODE_PUP_FORCE_SENSOR__FRAW, (uint8_t **)&raw);
    return *raw;
}

// pybricks.pupdevices.ForceSensor._force
STATIC int32_t pupdevices_ForceSensor__force(pupdevices_ForceSensor_obj_t *self) {
    // Get raw sensor value
    int32_t raw = pupdevices_ForceSensor__raw(self->iodev);

    // Get force in millinewtons
    int32_t force = (10000 * (raw - self->raw_released - self->raw_offset)) / (self->raw_end - self->raw_released);

    // With LEGO scaling, initial section is negative, so mask it and return
    return force < 0 ? 0 : force;
}

// pybricks.pupdevices.ForceSensor._distance
STATIC int32_t pupdevices_ForceSensor__distance(pupdevices_ForceSensor_obj_t *self) {
    int32_t raw = pupdevices_ForceSensor__raw(self->iodev);

    // Get distance in micrometers
    return (6670 * (raw - self->raw_released)) / (self->raw_end - self->raw_released);
}

// pybricks.pupdevices.ForceSensor.__init__
STATIC mp_obj_t pupdevices_ForceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ForceSensor_obj_t *self = mp_obj_malloc(pupdevices_ForceSensor_obj_t, type);

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevice.
    self->iodev = pb_pup_device_get_device(port, PBIO_IODEV_TYPE_ID_SPIKE_FORCE_SENSOR);

    // Read scaling factors.
    int16_t *calib;
    pb_pup_device_get_data(self->iodev, PBIO_IODEV_MODE_PUP_FORCE_SENSOR__CALIB, (uint8_t **)&calib);
    self->raw_offset = calib[1];
    self->raw_released = calib[2];
    self->raw_end = calib[6];

    // Do sanity check on values to verify calibration read succeeded
    if (self->raw_released >= self->raw_end) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    // Do one measurement to set up mode used for all methods.
    pupdevices_ForceSensor__raw(self->iodev);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ForceSensor.touched
STATIC mp_obj_t pupdevices_ForceSensor_touched(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return true if raw value is just above detectable change, with a small
    // margin to account for small calibration tolerances.
    return mp_obj_new_bool(pupdevices_ForceSensor__raw(self->iodev) > self->raw_released + 4);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ForceSensor_touched_obj, pupdevices_ForceSensor_touched);

// pybricks.pupdevices.ForceSensor.force
STATIC mp_obj_t pupdevices_ForceSensor_force(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return force in newtons
    return pb_obj_new_fraction(pupdevices_ForceSensor__force(self), 1000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ForceSensor_force_obj, pupdevices_ForceSensor_force);

// pybricks.pupdevices.ForceSensor.distance
STATIC mp_obj_t pupdevices_ForceSensor_distance(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return in millimeters
    return pb_obj_new_fraction(pupdevices_ForceSensor__distance(self), 1000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_ForceSensor_distance_obj, pupdevices_ForceSensor_distance);

// pybricks.pupdevices.ForceSensor.pressed
STATIC mp_obj_t pupdevices_ForceSensor_pressed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ForceSensor_obj_t, self,
        PB_ARG_DEFAULT_INT(force, 3));

    #if MICROPY_PY_BUILTINS_FLOAT
    int32_t f_arg = (int32_t)(mp_obj_get_float(force_in) * 1000);
    #else
    int32_t f_arg = pb_obj_get_int(force_in) * 1000;
    #endif

    // Return true if the force is bigger than given threshold
    return mp_obj_new_bool(pupdevices_ForceSensor__force(self) >= f_arg);
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_ForceSensor_pressed_obj, 1, pupdevices_ForceSensor_pressed);

// dir(pybricks.pupdevices.ForceSensor)
STATIC const mp_rom_map_elem_t pupdevices_ForceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_touched),     MP_ROM_PTR(&pupdevices_ForceSensor_touched_obj)              },
    { MP_ROM_QSTR(MP_QSTR_force),       MP_ROM_PTR(&pupdevices_ForceSensor_force_obj)                },
    { MP_ROM_QSTR(MP_QSTR_pressed),     MP_ROM_PTR(&pupdevices_ForceSensor_pressed_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&pupdevices_ForceSensor_distance_obj)             },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_ForceSensor_locals_dict, pupdevices_ForceSensor_locals_dict_table);

// type(pybricks.pupdevices.ForceSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_ForceSensor,
    MP_QSTR_ForceSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_ForceSensor_make_new,
    locals_dict, &pupdevices_ForceSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
