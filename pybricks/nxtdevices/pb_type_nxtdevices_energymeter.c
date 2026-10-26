// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/mphal.h"

#include <pbio/port.h>
#include <pbio/port_i2c_known.h>
#include <pbio/util.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.EnergyMeter class object
typedef struct _nxtdevices_EnergyMeter_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
} nxtdevices_EnergyMeter_obj_t;

// pybricks.nxtdevices.EnergyMeter.__init__
static mp_obj_t nxtdevices_EnergyMeter_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_EnergyMeter_obj_t *self = mp_obj_malloc(nxtdevices_EnergyMeter_obj_t, type);
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(pbio_port_get_port(port_id, &self->port));

    pb_module_tools_assert_blocking();

    pb_device_set_lego_mode(self->port);

    // Assert that the device is there and wait for its first sample.
    pbio_error_t err;
    const uint8_t *data;
    while ((err = pbio_port_get_i2c_known_data(self->port, LEGO_DEVICE_TYPE_ID_NXT_ENERGY_METER, &data)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(10);
    }
    pb_assert(err);

    return MP_OBJ_FROM_PTR(self);
}

/**
 * Gets one of the energy meter values.
 *
 * Values are little endian 16-bit registers in this order: input voltage (mV),
 * input current (mA), output voltage (mV), output current (mA), stored energy
 * (J), input power (mW), output power (mW).
 *
 * @param [in]  data        The raw register data.
 * @param [in]  index       Index of the value.
 * @return                  The value.
 */
static mp_obj_t nxtdevices_EnergyMeter_get_value(const uint8_t *data, uint8_t index) {
    return mp_obj_new_int((int16_t)pbio_get_uint16_le(&data[index * sizeof(int16_t)]));
}

/**
 * Gets three of the energy meter values as a tuple.
 *
 * @param [in]  self_in     The energy meter object.
 * @param [in]  i0, i1, i2  Indexes of the values.
 */
static mp_obj_t nxtdevices_EnergyMeter_get_tuple(mp_obj_t self_in, uint8_t i0, uint8_t i1, uint8_t i2) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const uint8_t *data;
    pb_assert(pbio_port_get_i2c_known_data(self->port, LEGO_DEVICE_TYPE_ID_NXT_ENERGY_METER, &data));
    mp_obj_t dat[] = {
        nxtdevices_EnergyMeter_get_value(data, i0),
        nxtdevices_EnergyMeter_get_value(data, i1),
        nxtdevices_EnergyMeter_get_value(data, i2),
    };
    return mp_obj_new_tuple(MP_ARRAY_SIZE(dat), dat);
}

// pybricks.nxtdevices.EnergyMeter.input
static mp_obj_t nxtdevices_EnergyMeter_input(mp_obj_t self_in) {
    return nxtdevices_EnergyMeter_get_tuple(self_in, 0, 1, 5);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_input_obj, nxtdevices_EnergyMeter_input);

// pybricks.nxtdevices.EnergyMeter.storage
static mp_obj_t nxtdevices_EnergyMeter_storage(mp_obj_t self_in) {
    nxtdevices_EnergyMeter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const uint8_t *data;
    pb_assert(pbio_port_get_i2c_known_data(self->port, LEGO_DEVICE_TYPE_ID_NXT_ENERGY_METER, &data));
    return nxtdevices_EnergyMeter_get_value(data, 4);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_EnergyMeter_storage_obj, nxtdevices_EnergyMeter_storage);

// pybricks.nxtdevices.EnergyMeter.output
static mp_obj_t nxtdevices_EnergyMeter_output(mp_obj_t self_in) {
    return nxtdevices_EnergyMeter_get_tuple(self_in, 2, 3, 6);
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
