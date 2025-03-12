// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include <pbio/int_math.h>
#include <pbio/port_interface.h>

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.LightSensor class object
typedef struct _nxtdevices_LightSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
} nxtdevices_LightSensor_obj_t;

static const pbio_int_math_point_t ambient_slope[] = {
    { .x = 500, .y = 1000 },
    { .x = 4000, .y = 0 },
};

// pybricks.nxtdevices.LightSensor.ambient
static mp_obj_t nxtdevices_LightSensor_ambient(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pbio_port_dcm_analog_rgba_t *rgba;
    pb_assert(pbio_port_get_analog_rgba(self->port, LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR, &rgba));
    return pb_obj_new_fraction(pbio_int_math_interpolate(ambient_slope, MP_ARRAY_SIZE(ambient_slope), rgba->a), 10);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_ambient_obj, nxtdevices_LightSensor_ambient);

static const pbio_int_math_point_t reflection_slope[] = {
    { .x = 400, .y = 0 },
    { .x = 900, .y = 900 },
    { .x = 1300, .y = 1000 },
};

// pybricks.nxtdevices.LightSensor.reflection
static mp_obj_t nxtdevices_LightSensor_reflection(mp_obj_t self_in) {
    nxtdevices_LightSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_port_dcm_analog_rgba_t *rgba;
    pb_assert(pbio_port_get_analog_rgba(self->port, LEGO_DEVICE_TYPE_ID_NXT_LIGHT_SENSOR, &rgba));

    // Both values are inverted, so this really computes reflected - ambient.
    uint32_t offset = rgba->r <= rgba->a ? rgba->a - rgba->r : 0;

    return pb_obj_new_fraction(pbio_int_math_interpolate(reflection_slope, MP_ARRAY_SIZE(reflection_slope), offset), 10);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_LightSensor_reflection_obj, nxtdevices_LightSensor_reflection);

// pybricks.nxtdevices.LightSensor.__init__
static mp_obj_t nxtdevices_LightSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_LightSensor_obj_t *self = mp_obj_malloc(nxtdevices_LightSensor_obj_t, type);
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(pbio_port_get_port(port_id, &self->port));

    // Measure once. This will assert that the device is there.
    mp_obj_t self_obj = MP_OBJ_FROM_PTR(self);
    nxtdevices_LightSensor_reflection(self_obj);
    return self_obj;
}

// dir(pybricks.nxtdevices.LightSensor)
static const mp_rom_map_elem_t nxtdevices_LightSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_ambient),  MP_ROM_PTR(&nxtdevices_LightSensor_ambient_obj) },
    { MP_ROM_QSTR(MP_QSTR_reflection), MP_ROM_PTR(&nxtdevices_LightSensor_reflection_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_LightSensor_locals_dict, nxtdevices_LightSensor_locals_dict_table);

// type(pybricks.nxtdevices.LightSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_LightSensor,
    MP_QSTR_LightSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_LightSensor_make_new,
    locals_dict, &nxtdevices_LightSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
