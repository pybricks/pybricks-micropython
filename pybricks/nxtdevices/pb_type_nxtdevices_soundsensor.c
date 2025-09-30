// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include <pbio/int_math.h>
#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.SoundSensor class object
typedef struct _nxtdevices_SoundSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
} nxtdevices_SoundSensor_obj_t;

// Generic linear scaling of an analog value between a known min and max to a percentage
static int32_t analog_scale(int32_t mvolts, int32_t mvolts_min, int32_t mvolts_max, bool invert) {
    int32_t scaled = (100 * (mvolts - mvolts_min)) / (mvolts_max - mvolts_min);
    if (invert) {
        scaled = 100 - scaled;
    }
    return pbio_int_math_bind(scaled, 0, 100);
}

// pybricks.nxtdevices.SoundSensor.intensity
static mp_obj_t nxtdevices_SoundSensor_intensity(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        nxtdevices_SoundSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(audible_only));

    uint32_t analog;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_SOUND_SENSOR, mp_obj_is_true(audible_only_in), &analog));
    return mp_obj_new_int(analog_scale(analog, 650, 4860, true));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(nxtdevices_SoundSensor_intensity_obj, 1, nxtdevices_SoundSensor_intensity);

// pybricks.nxtdevices.SoundSensor.__init__
static mp_obj_t nxtdevices_SoundSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_SoundSensor_obj_t *self = mp_obj_malloc(nxtdevices_SoundSensor_obj_t, type);
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(pbio_port_get_port(port_id, &self->port));

    // Do one reading for consistent initial mode
    mp_obj_t pos_args[] = { MP_OBJ_FROM_PTR(self) };
    nxtdevices_SoundSensor_intensity(1, pos_args, (mp_map_t *)&mp_const_empty_map);
    return MP_OBJ_FROM_PTR(self);
}

// dir(pybricks.nxtdevices.SoundSensor)
static const mp_rom_map_elem_t nxtdevices_SoundSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_intensity), MP_ROM_PTR(&nxtdevices_SoundSensor_intensity_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_SoundSensor_locals_dict, nxtdevices_SoundSensor_locals_dict_table);

// type(pybricks.nxtdevices.SoundSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_SoundSensor,
    MP_QSTR_SoundSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_SoundSensor_make_new,
    locals_dict, &nxtdevices_SoundSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
