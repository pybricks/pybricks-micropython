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
} pupdevices_ForceSensor_obj_t;

// pybricks.pupdevices.ForceSensor.__init__
static mp_obj_t pupdevices_ForceSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // This device only ever uses one mode at runtime, but this class was
    // historically async, so each method returns an awaitable constant in
    // async mode.
    return pb_type_device_make_new(type, n_args, n_kw, args, sizeof(pupdevices_ForceSensor_obj_t), LEGO_DEVICE_TYPE_ID_SPIKE_FORCE_SENSOR);
}

// pybricks.pupdevices.ForceSensor.touched
static mp_obj_t get_touched(mp_obj_t self_in) {
    // Return true if raw value is just above detectable change, with a small
    // margin to account for small calibration tolerances.
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t force;
    int32_t distance;
    pb_assert(pbio_port_lump_get_force(self->device_base.lump_dev, &force, &distance));
    mp_obj_t result = mp_obj_new_bool(distance > 100);
    return pb_type_async_return_result(result, &self->device_base.last_awaitable);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_touched_obj, LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__FRAW, get_touched);

// pybricks.pupdevices.ForceSensor.force
static mp_obj_t get_force(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t force;
    int32_t distance;
    pb_assert(pbio_port_lump_get_force(self->device_base.lump_dev, &force, &distance));
    mp_obj_t result = pb_obj_new_fraction(force, 1000);
    return pb_type_async_return_result(result, &self->device_base.last_awaitable);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_force_obj, LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__FRAW, get_force);

// pybricks.pupdevices.ForceSensor.distance
static mp_obj_t get_distance(mp_obj_t self_in) {
    pupdevices_ForceSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t force;
    int32_t distance;
    pb_assert(pbio_port_lump_get_force(self->device_base.lump_dev, &force, &distance));
    mp_obj_t result = pb_obj_new_fraction(distance, 1000);
    return pb_type_async_return_result(result, &self->device_base.last_awaitable);
}
static PB_DEFINE_CONST_TYPE_DEVICE_METHOD_OBJ(get_distance_obj, LEGO_DEVICE_MODE_PUP_FORCE_SENSOR__FRAW, get_distance);

// pybricks.pupdevices.ForceSensor.pressed
static mp_obj_t get_pressed(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_ForceSensor_obj_t, self,
        PB_ARG_DEFAULT_INT(force, 3));

    #if MICROPY_PY_BUILTINS_FLOAT
    int32_t pressed_threshold = (int32_t)(mp_obj_get_float(force_in) * 1000);
    #else
    int32_t pressed_threshold = pb_obj_get_int(force_in) * 1000;
    #endif

    int32_t force;
    int32_t distance;
    pb_assert(pbio_port_lump_get_force(self->device_base.lump_dev, &force, &distance));
    mp_obj_t result = mp_obj_new_bool(force > pressed_threshold);
    return pb_type_async_return_result(result, &self->device_base.last_awaitable);
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
