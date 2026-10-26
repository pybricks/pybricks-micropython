// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2026 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include "py/mphal.h"

#include <pbio/port.h>
#include <pbio/port_i2c_known.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.UltrasonicSensor class object
typedef struct _nxtdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
} nxtdevices_UltrasonicSensor_obj_t;

// pybricks.nxtdevices.UltrasonicSensor.__init__
static mp_obj_t nxtdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_UltrasonicSensor_obj_t *self = mp_obj_malloc(nxtdevices_UltrasonicSensor_obj_t, type);
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(pbio_port_get_port(port_id, &self->port));

    pb_module_tools_assert_blocking();

    pb_device_set_lego_mode(self->port);

    // Assert that the device is there and wait for its first sample.
    pbio_error_t err;
    const uint8_t *data;
    while ((err = pbio_port_get_i2c_known_data(self->port, LEGO_DEVICE_TYPE_ID_NXT_ULTRASONIC_SENSOR, &data)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(10);
    }
    pb_assert(err);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.UltrasonicSensor.distance
static mp_obj_t nxtdevices_UltrasonicSensor_distance(mp_obj_t self_in) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const uint8_t *data;
    pb_assert(pbio_port_get_i2c_known_data(self->port, LEGO_DEVICE_TYPE_ID_NXT_ULTRASONIC_SENSOR, &data));
    // Distance register is in cm.
    return mp_obj_new_int(data[0] * 10);
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
