// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/mphal.h"

#include <pbdrv/i2c.h>
#include <pbio/port_interface.h>
#include <pbio/util.h>

#include <pybricks/common.h>
#include <pybricks/iodevices/iodevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.EnergyMeter class object
typedef struct _nxtdevices_EnergyMeter_obj_t {
    mp_obj_base_t base;
    mp_obj_t i2c_device_obj;
} nxtdevices_EnergyMeter_obj_t;

// pybricks.nxtdevices.EnergyMeter.__init__
static mp_obj_t nxtdevices_EnergyMeter_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_EnergyMeter_obj_t *self = mp_obj_malloc(nxtdevices_EnergyMeter_obj_t, type);
    self->i2c_device_obj = pb_type_i2c_device_make_new(MP_OBJ_FROM_PTR(self), port_in, 0x02, true, false, false);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t nxtdevices_EnergyMeter_read_all(mp_obj_t self_in, pb_type_i2c_device_return_map_t return_map) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const uint8_t reg_data[] = { 0x0A };
    return pb_type_i2c_device_start_operation(self->i2c_device_obj, reg_data, MP_ARRAY_SIZE(reg_data), 14, return_map);
}

// pybricks.nxtdevices.EnergyMeter.input
static mp_obj_t map_input(mp_obj_t self_in, const uint8_t *all, size_t len) {
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int((int16_t)pbio_get_uint16_le(&all[0]));
    dat[1] = mp_obj_new_int((int16_t)pbio_get_uint16_le(&all[2]));
    dat[2] = mp_obj_new_int((int16_t)pbio_get_uint16_le(&all[10]));
    return mp_obj_new_tuple(MP_ARRAY_SIZE(dat), dat);
}

static mp_obj_t nxtdevices_EnergyMeter_input(mp_obj_t self_in) {
    return nxtdevices_EnergyMeter_read_all(self_in, map_input);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_input_obj, nxtdevices_EnergyMeter_input);

// pybricks.nxtdevices.EnergyMeter.storage
static mp_obj_t map_storage(mp_obj_t self_in, const uint8_t *all, size_t len) {
    return mp_obj_new_int((int16_t)pbio_get_uint16_le(&all[8]));
}

static mp_obj_t nxtdevices_EnergyMeter_storage(mp_obj_t self_in) {
    return nxtdevices_EnergyMeter_read_all(self_in, map_storage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_storage_obj, nxtdevices_EnergyMeter_storage);

// pybricks.nxtdevices.EnergyMeter.output
static mp_obj_t map_output(mp_obj_t self_in, const uint8_t *all, size_t len) {
    mp_obj_t dat[3];
    dat[0] = mp_obj_new_int((int16_t)pbio_get_uint16_le(&all[4]));
    dat[1] = mp_obj_new_int((int16_t)pbio_get_uint16_le(&all[6]));
    dat[2] = mp_obj_new_int((int16_t)pbio_get_uint16_le(&all[12]));
    return mp_obj_new_tuple(MP_ARRAY_SIZE(dat), dat);
}

static mp_obj_t nxtdevices_EnergyMeter_output(mp_obj_t self_in) {
    return nxtdevices_EnergyMeter_read_all(self_in, map_output);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_output_obj, nxtdevices_EnergyMeter_output);

// dir(pybricks.nxtdevices.EnergyMeter)
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


#endif // PYBRICKS_PY_NXTDEVICES
