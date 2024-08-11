// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbio/int_math.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for ForceSensor
typedef struct _pupdevices_ForceSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    int32_t raw_released;
    int32_t raw_offset;
    int32_t raw_start;
    int32_t raw_end;
    int32_t pressed_threshold;
} pupdevices_ForceSensor_obj_t;

// pybricks.pupdevices.ForceSensor.__init__
static mp_obj_t pupdevices_ForceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_ForceSensor_obj_t *self = mp_obj_malloc(pupdevices_ForceSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_SPIKE_FORCE_SENSOR);

    // Read scaling factors.
    int16_t *calib = pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__CALIB);
    self->raw_offset = calib[1];
    self->raw_released = calib[2];
    self->raw_end = calib[6];
    self->pressed_threshold = 0;

    // Do sanity check on values to verify calibration read succeeded
    if (self->raw_released >= self->raw_end) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    // Do one measurement to set up mode used for all methods.
    pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__FRAW);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.ForceSensor._raw
static int32_t get_raw(mp_obj_t self_in) {
    int16_t *raw = pb_type_device_get_data(self_in, PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__FRAW);
    return *raw;
}

// pybricks.pupdevices.ForceSensor._force
static int32_t get_force_mN(mp_obj_t self_in) {
    // Get force in millinewtons
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t force = (10000 * (get_raw(self_in) - self->raw_released - self->raw_offset)) / (self->raw_end - self->raw_released);
    // With LEGO scaling, initial section is negative, so mask it and return
    return force < 0 ? 0 : force;
}

// pybricks.pupdevices.ForceSensor.touched
static mp_obj_t get_touched(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // Return true if raw value is just above detectable change, with a small
    // margin to account for small calibration tolerances.
    return mp_obj_new_bool(get_raw(self_in) > self->raw_released + 4);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_touched_obj, PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__FRAW, get_touched);

// pybricks.pupdevices.ForceSensor.force
static mp_obj_t get_force(mp_obj_t self_in) {
    return pb_obj_new_fraction(get_force_mN(self_in), 1000);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_force_obj, PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__FRAW, get_force);

// pybricks.pupdevices.ForceSensor.distance
static mp_obj_t get_distance(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t distance_um = (6670 * (get_raw(self_in) - self->raw_released)) / (self->raw_end - self->raw_released);
    return pb_obj_new_fraction(distance_um, 1000);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_distance_obj, PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__FRAW, get_distance);

// pybricks.pupdevices.ForceSensor.pressed(force=default)
static mp_obj_t get_pressed_simple(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(get_force_mN(self_in) >= self->pressed_threshold);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_pressed_simple_obj, PBDRV_LEGODEV_MODE_PUP_FORCE_SENSOR__FRAW, get_pressed_simple);

// pybricks.pupdevices.ForceSensor.pressed
static mp_obj_t get_pressed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ForceSensor_obj_t, self,
        PB_ARG_DEFAULT_INT(force, 3));

    #if MICROPY_PY_BUILTINS_FLOAT
    self->pressed_threshold = (int32_t)(mp_obj_get_float(force_in) * 1000);
    #else
    self->pressed_threshold = pb_obj_get_int(force_in) * 1000;
    #endif

    return pb_type_device_method_call(MP_OBJ_FROM_PTR(&get_pressed_simple_obj), 1, 0, pos_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(get_pressed_obj, 1, get_pressed);

// dir(pybricks.pupdevices.ForceSensor)
static const mp_rom_map_elem_t pupdevices_ForceSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_touched),     MP_ROM_PTR(&get_touched_obj)              },
    { MP_ROM_QSTR(MP_QSTR_force),       MP_ROM_PTR(&get_force_obj)                },
    { MP_ROM_QSTR(MP_QSTR_pressed),     MP_ROM_PTR(&get_pressed_obj)              },
    { MP_ROM_QSTR(MP_QSTR_distance),    MP_ROM_PTR(&get_distance_obj)             },
};
static MP_DEFINE_CONST_DICT(pupdevices_ForceSensor_locals_dict, pupdevices_ForceSensor_locals_dict_table);

// type(pybricks.pupdevices.ForceSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_ForceSensor,
    MP_QSTR_ForceSensor,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_ForceSensor_make_new,
    locals_dict, &pupdevices_ForceSensor_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
