// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES

#include <pybricks/common.h>
#include <pybricks/nxtdevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_device.h>
// pybricks.nxtdevices.EnergyMeter class object
typedef struct _nxtdevices_EnergyMeter_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_EnergyMeter_obj_t;

// pybricks.nxtdevices.EnergyMeter.__init__
STATIC mp_obj_t nxtdevices_EnergyMeter_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_EnergyMeter_obj_t *self = m_new_obj(nxtdevices_EnergyMeter_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port, PBIO_IODEV_TYPE_ID_NXT_ENERGY_METER);

    // Read once so we are in the mode we'll be using for all methods, to avoid mode switch delays later
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.EnergyMeter.storage
STATIC mp_obj_t nxtdevices_EnergyMeter_storage(mp_obj_t self_in) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);
    return mp_obj_new_int(all[4]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_storage_obj, nxtdevices_EnergyMeter_storage);

// pybricks.nxtdevices.EnergyMeter.input
STATIC mp_obj_t nxtdevices_EnergyMeter_input(mp_obj_t self_in) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int(all[0]);
    dat[1] = mp_obj_new_int(all[1]);
    dat[2] = mp_obj_new_int(all[5]);
    return mp_obj_new_tuple(3, dat);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_input_obj, nxtdevices_EnergyMeter_input);

// pybricks.nxtdevices.EnergyMeter.output
STATIC mp_obj_t nxtdevices_EnergyMeter_output(mp_obj_t self_in) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t all[7];
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ENERGY_METER_ALL, all);
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int(all[2]);
    dat[1] = mp_obj_new_int(all[3]);
    dat[2] = mp_obj_new_int(all[6]);
    return mp_obj_new_tuple(3, dat);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_output_obj, nxtdevices_EnergyMeter_output);

// dir(pybricks.ev3devices.EnergyMeter)
STATIC const mp_rom_map_elem_t nxtdevices_EnergyMeter_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_input),      MP_ROM_PTR(&nxtdevices_EnergyMeter_input_obj) },
    { MP_ROM_QSTR(MP_QSTR_output),     MP_ROM_PTR(&nxtdevices_EnergyMeter_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_storage),    MP_ROM_PTR(&nxtdevices_EnergyMeter_storage_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_EnergyMeter_locals_dict, nxtdevices_EnergyMeter_locals_dict_table);

// type(pybricks.nxtdevices.EnergyMeter)
const mp_obj_type_t pb_type_nxtdevices_EnergyMeter = {
    { &mp_type_type },
    .name = MP_QSTR_EnergyMeter,
    .make_new = nxtdevices_EnergyMeter_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_EnergyMeter_locals_dict,
};


#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
