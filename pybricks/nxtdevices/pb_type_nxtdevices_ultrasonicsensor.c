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

// pybricks.nxtdevices.UltrasonicSensor class object
typedef struct _nxtdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    mp_obj_t *i2c_device_obj;
} nxtdevices_UltrasonicSensor_obj_t;

// pybricks.nxtdevices.UltrasonicSensor.__init__
static mp_obj_t nxtdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_UltrasonicSensor_obj_t *self = mp_obj_malloc(nxtdevices_UltrasonicSensor_obj_t, type);
    self->i2c_device_obj = pb_type_i2c_device_make_new(port_in, 0x01, false, true, true);

    // NXT Ultrasonic Sensor appears to need some time after initializing I2C pins before it can receive data.
    mp_hal_delay_ms(100);

    pb_type_i2c_device_assert_string_at_register(self->i2c_device_obj, 0x08, "LEGO");
    pb_type_i2c_device_assert_string_at_register(self->i2c_device_obj, 0x10, "Sonar");

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t map_distance(const uint8_t *data, size_t len) {
    return mp_obj_new_int(data[0] * 10);
}

// pybricks.nxtdevices.UltrasonicSensor.distance
static mp_obj_t nxtdevices_UltrasonicSensor_distance(mp_obj_t self_in) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const uint8_t write_data[] = { 0x42 };
    return pb_type_i2c_device_start_operation(self->i2c_device_obj, write_data, MP_ARRAY_SIZE(write_data), 1, map_distance);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_UltrasonicSensor_distance_obj, nxtdevices_UltrasonicSensor_distance);

// dir(pybricks.nxtdevices.UltrasonicSensor)
static const mp_rom_map_elem_t nxtdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_distance_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_UltrasonicSensor_locals_dict, nxtdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.nxtdevices.UltrasonicSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_UltrasonicSensor,
    MP_QSTR_UltrasonicSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_UltrasonicSensor_make_new,
    locals_dict, &nxtdevices_UltrasonicSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
