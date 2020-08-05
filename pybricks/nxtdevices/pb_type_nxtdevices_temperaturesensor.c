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

// pybricks.nxtdevices.TemperatureSensor class object
typedef struct _nxtdevices_TemperatureSensor_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} nxtdevices_TemperatureSensor_obj_t;

// pybricks.nxtdevices.TemperatureSensor.__init__
STATIC mp_obj_t nxtdevices_TemperatureSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_TemperatureSensor_obj_t *self = m_new_obj(nxtdevices_TemperatureSensor_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_TEMPERATURE_SENSOR);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.TemperatureSensor.temperature
STATIC mp_obj_t nxtdevices_TemperatureSensor_temperature(mp_obj_t self_in) {
    nxtdevices_TemperatureSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t temperature_scaled;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_TEMPERATURE_SENSOR_CELCIUS, &temperature_scaled);
    return mp_obj_new_float((temperature_scaled >> 4) / 16.0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_TemperatureSensor_temperature_obj, nxtdevices_TemperatureSensor_temperature);

// dir(pybricks.ev3devices.TemperatureSensor)
STATIC const mp_rom_map_elem_t nxtdevices_TemperatureSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_temperature),    MP_ROM_PTR(&nxtdevices_TemperatureSensor_temperature_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_TemperatureSensor_locals_dict, nxtdevices_TemperatureSensor_locals_dict_table);

// type(pybricks.nxtdevices.TemperatureSensor)
const mp_obj_type_t pb_type_nxtdevices_TemperatureSensor = {
    { &mp_type_type },
    .name = MP_QSTR_TemperatureSensor,
    .make_new = nxtdevices_TemperatureSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&nxtdevices_TemperatureSensor_locals_dict,
};

#endif // PYBRICKS_PY_NXTDEVICES && PYBRICKS_PY_EV3DEVICES
