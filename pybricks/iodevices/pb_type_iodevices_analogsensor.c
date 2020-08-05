// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES

#include <string.h>

#include <pbio/iodev.h>
#include <pbio/serial.h>

#include "py/mpconfig.h"

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objstr.h"

#include <pybricks/util_pb/pb_device.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_pb/pb_error.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

// pybricks.iodevices.AnalogSensor class object
typedef struct _iodevices_AnalogSensor_obj_t {
    mp_obj_base_t base;
    bool active;
    pb_device_t *pbdev;
} iodevices_AnalogSensor_obj_t;

// pybricks.iodevices.AnalogSensor.__init__
STATIC mp_obj_t iodevices_AnalogSensor_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));
    iodevices_AnalogSensor_obj_t *self = m_new_obj(iodevices_AnalogSensor_obj_t);
    self->base.type = (mp_obj_type_t *)otype;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_NXT_ANALOG);

    // Initialize NXT sensors to passive state
    int32_t voltage;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.AnalogSensor.voltage
STATIC mp_obj_t iodevices_AnalogSensor_voltage(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pb_device_get_values(self->pbdev, mode, &voltage);
    return mp_obj_new_int(voltage);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_voltage_obj, iodevices_AnalogSensor_voltage);

// pybricks.iodevices.AnalogSensor.resistance
STATIC mp_obj_t iodevices_AnalogSensor_resistance(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pb_device_get_values(self->pbdev, mode, &voltage);

    // Open terminal/infinite resistance, return infinite resistance
    const int32_t vmax = 4972;
    if (voltage >= vmax) {
        return mp_obj_new_int(MP_SSIZE_MAX);
    }
    // Return as if a pure voltage divider between load and 10K internal resistor
    return mp_obj_new_int((10000 * voltage) / (vmax - voltage));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_resistance_obj, iodevices_AnalogSensor_resistance);

// pybricks.iodevices.AnalogSensor.active
STATIC mp_obj_t iodevices_AnalogSensor_active(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE, &voltage);
    self->active = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_active_obj, iodevices_AnalogSensor_active);

// pybricks.iodevices.AnalogSensor.passive
STATIC mp_obj_t iodevices_AnalogSensor_passive(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pb_device_get_values(self->pbdev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage);
    self->active = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_passive_obj, iodevices_AnalogSensor_passive);

// dir(pybricks.iodevices.AnalogSensor)
STATIC const mp_rom_map_elem_t iodevices_AnalogSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_voltage),    MP_ROM_PTR(&iodevices_AnalogSensor_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_resistance), MP_ROM_PTR(&iodevices_AnalogSensor_resistance_obj) },
    { MP_ROM_QSTR(MP_QSTR_active),     MP_ROM_PTR(&iodevices_AnalogSensor_active_obj)    },
    { MP_ROM_QSTR(MP_QSTR_passive),    MP_ROM_PTR(&iodevices_AnalogSensor_passive_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(iodevices_AnalogSensor_locals_dict, iodevices_AnalogSensor_locals_dict_table);

// type(pybricks.iodevices.AnalogSensor)
const mp_obj_type_t pb_type_iodevices_AnalogSensor = {
    { &mp_type_type },
    .name = MP_QSTR_AnalogSensor,
    .make_new = iodevices_AnalogSensor_make_new,
    .locals_dict = (mp_obj_dict_t *)&iodevices_AnalogSensor_locals_dict,
};

#endif // PYBRICKS_PY_IODEVICES
