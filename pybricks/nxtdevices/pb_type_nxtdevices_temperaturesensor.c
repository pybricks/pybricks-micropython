// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/mphal.h"

#include <pbdrv/i2c.h>
#include <pbio/port_interface.h>

#include <pybricks/common.h>
#include <pybricks/iodevices/iodevices.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.TemperatureSensor class object
typedef struct _nxtdevices_TemperatureSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t i2c_device_obj;
} nxtdevices_TemperatureSensor_obj_t;

// pybricks.nxtdevices.TemperatureSensor.__init__
static mp_obj_t nxtdevices_TemperatureSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_TemperatureSensor_obj_t *self = mp_obj_malloc(nxtdevices_TemperatureSensor_obj_t, type);
    self->i2c_device_obj = pb_type_i2c_device_make_new(MP_OBJ_FROM_PTR(self), port_in, 0x4C, true, false, false);

    // Set resolution to 0.125 degrees as a fair balance between speed and accuracy.
    const uint8_t write_data[] = { 0x01, (1 << 6) | (0 << 5) };
    pb_type_i2c_device_start_operation(self->i2c_device_obj, write_data, MP_ARRAY_SIZE(write_data), 0, NULL);

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t map_temperature(mp_obj_t self_in, const uint8_t *data, size_t len) {
    int16_t combined = ((uint16_t)data[0] << 8) | data[1];
    return mp_obj_new_float_from_f((combined >> 4) / 16.0f);
}

// pybricks.nxtdevices.TemperatureSensor.temperature
static mp_obj_t nxtdevices_TemperatureSensor_temperature(mp_obj_t self_in) {
    nxtdevices_TemperatureSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const uint8_t write_data[] = { 0x00 };
    return pb_type_i2c_device_start_operation(self->i2c_device_obj, write_data, MP_ARRAY_SIZE(write_data), 2, map_temperature);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_TemperatureSensor_temperature_obj, nxtdevices_TemperatureSensor_temperature);

// dir(pybricks.nxtdevices.TemperatureSensor)
static const mp_rom_map_elem_t nxtdevices_TemperatureSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&nxtdevices_TemperatureSensor_temperature_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_TemperatureSensor_locals_dict, nxtdevices_TemperatureSensor_locals_dict_table);

// type(pybricks.nxtdevices.TemperatureSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_TemperatureSensor,
    MP_QSTR_TemperatureSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_TemperatureSensor_make_new,
    locals_dict, &nxtdevices_TemperatureSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
