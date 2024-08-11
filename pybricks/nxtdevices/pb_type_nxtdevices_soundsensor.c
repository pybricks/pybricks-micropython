// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>

// pybricks.nxtdevices.SoundSensor class object
typedef struct _nxtdevices_SoundSensor_obj_t {
    pb_type_device_obj_base_t device_base;
} nxtdevices_SoundSensor_obj_t;

// pybricks.nxtdevices.SoundSensor.intensity
static mp_obj_t nxtdevices_SoundSensor_intensity(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        nxtdevices_SoundSensor_obj_t, self,
        PB_ARG_DEFAULT_TRUE(audible_only));

    uint8_t mode = mp_obj_is_true(audible_only_in) ? PBDRV_LEGODEV_MODE_NXT_ANALOG__ACTIVE : PBDRV_LEGODEV_MODE_NXT_ANALOG__PASSIVE;
    int32_t *analog = pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), mode);

    return mp_obj_new_int(analog_scale(analog[0], 650, 4860, true));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(nxtdevices_SoundSensor_intensity_obj, 1, nxtdevices_SoundSensor_intensity);

// pybricks.nxtdevices.SoundSensor.__init__
static mp_obj_t nxtdevices_SoundSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_SoundSensor_obj_t *self = mp_obj_malloc(nxtdevices_SoundSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_NXT_SOUND_SENSOR);

    // Do one reading for consistent initial mode
    mp_obj_t pos_args[1] = { self };
    mp_map_t kwd_args;
    mp_map_init_fixed_table(&kwd_args, 0, NULL);
    nxtdevices_SoundSensor_intensity(1, pos_args, &kwd_args);

    return MP_OBJ_FROM_PTR(self);
}

// dir(pybricks.ev3devices.SoundSensor)
static const mp_rom_map_elem_t nxtdevices_SoundSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_intensity),  MP_ROM_PTR(&nxtdevices_SoundSensor_intensity_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_SoundSensor_locals_dict, nxtdevices_SoundSensor_locals_dict_table);

// type(pybricks.ev3devices.SoundSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_SoundSensor,
    MP_QSTR_SoundSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_SoundSensor_make_new,
    locals_dict, &nxtdevices_SoundSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
