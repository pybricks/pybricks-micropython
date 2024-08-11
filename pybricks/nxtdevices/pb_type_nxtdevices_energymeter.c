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
// pybricks.nxtdevices.EnergyMeter class object
typedef struct _nxtdevices_EnergyMeter_obj_t {
    pb_type_device_obj_base_t device_base;
} nxtdevices_EnergyMeter_obj_t;

// pybricks.nxtdevices.EnergyMeter.__init__
static mp_obj_t nxtdevices_EnergyMeter_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_EnergyMeter_obj_t *self = mp_obj_malloc(nxtdevices_EnergyMeter_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_NXT_ENERGY_METER);

    // Read once so we are in the mode we'll be using for all methods, to avoid mode switch delays later
    pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_NXT_ENERGY_METER_ALL);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.EnergyMeter.storage
static mp_obj_t nxtdevices_EnergyMeter_storage(mp_obj_t self_in) {
    int32_t *all = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_NXT_ENERGY_METER_ALL);
    return mp_obj_new_int(all[4]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_storage_obj, nxtdevices_EnergyMeter_storage);

// pybricks.nxtdevices.EnergyMeter.input
static mp_obj_t nxtdevices_EnergyMeter_input(mp_obj_t self_in) {
    int32_t *all = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_NXT_ENERGY_METER_ALL);
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int(all[0]);
    dat[1] = mp_obj_new_int(all[1]);
    dat[2] = mp_obj_new_int(all[5]);
    return mp_obj_new_tuple(3, dat);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_input_obj, nxtdevices_EnergyMeter_input);

// pybricks.nxtdevices.EnergyMeter.output
static mp_obj_t nxtdevices_EnergyMeter_output(mp_obj_t self_in) {
    int32_t *all = pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_NXT_ENERGY_METER_ALL);
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int(all[2]);
    dat[1] = mp_obj_new_int(all[3]);
    dat[2] = mp_obj_new_int(all[6]);
    return mp_obj_new_tuple(3, dat);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_output_obj, nxtdevices_EnergyMeter_output);

// dir(pybricks.ev3devices.EnergyMeter)
static const mp_rom_map_elem_t nxtdevices_EnergyMeter_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_input),      MP_ROM_PTR(&nxtdevices_EnergyMeter_input_obj) },
    { MP_ROM_QSTR(MP_QSTR_output),     MP_ROM_PTR(&nxtdevices_EnergyMeter_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_storage),    MP_ROM_PTR(&nxtdevices_EnergyMeter_storage_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_EnergyMeter_locals_dict, nxtdevices_EnergyMeter_locals_dict_table);

// type(pybricks.nxtdevices.EnergyMeter)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_EnergyMeter,
    MP_QSTR_EnergyMeter,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_EnergyMeter_make_new,
    locals_dict, &nxtdevices_EnergyMeter_locals_dict);


#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
