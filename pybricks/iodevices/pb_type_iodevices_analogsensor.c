// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES

#include "py/obj.h"
#include "py/smallint.h"

#include <pbdrv/legodev.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/common/pb_type_device.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.iodevices.AnalogSensor class object
typedef struct _iodevices_AnalogSensor_obj_t {
    pb_type_device_obj_base_t device_base;
    bool active;
} iodevices_AnalogSensor_obj_t;

// pybricks.iodevices.AnalogSensor.__init__
static mp_obj_t iodevices_AnalogSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    iodevices_AnalogSensor_obj_t *self = mp_obj_malloc(iodevices_AnalogSensor_obj_t, type);
    pb_type_device_init_class(&self->device_base, port_in, PBDRV_LEGODEV_TYPE_ID_NXT_ANALOG);

    // Initialize NXT sensors to passive state
    pb_type_device_get_data_blocking(MP_OBJ_FROM_PTR(self), PBDRV_LEGODEV_MODE_NXT_ANALOG__PASSIVE);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.AnalogSensor.voltage
static mp_obj_t iodevices_AnalogSensor_voltage(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t mode = self->active ? PBDRV_LEGODEV_MODE_NXT_ANALOG__ACTIVE : PBDRV_LEGODEV_MODE_NXT_ANALOG__PASSIVE;
    int32_t *voltage = pb_type_device_get_data_blocking(self_in, mode);
    return mp_obj_new_int(voltage[0]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_voltage_obj, iodevices_AnalogSensor_voltage);

// pybricks.iodevices.AnalogSensor.resistance
static mp_obj_t iodevices_AnalogSensor_resistance(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t mode = self->active ? PBDRV_LEGODEV_MODE_NXT_ANALOG__ACTIVE : PBDRV_LEGODEV_MODE_NXT_ANALOG__PASSIVE;
    int32_t voltage = *(int32_t *)pb_type_device_get_data_blocking(self_in, mode);

    // Open terminal/infinite resistance, return infinite resistance
    const int32_t vmax = 4972;
    if (voltage >= vmax) {
        return mp_obj_new_int(MP_SMALL_INT_MAX);
    }
    // Return as if a pure voltage divider between load and 10K internal resistor
    return mp_obj_new_int((10000 * voltage) / (vmax - voltage));
}
static MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_resistance_obj, iodevices_AnalogSensor_resistance);

// pybricks.iodevices.AnalogSensor.active
static mp_obj_t iodevices_AnalogSensor_active(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_NXT_ANALOG__ACTIVE);
    self->active = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_active_obj, iodevices_AnalogSensor_active);

// pybricks.iodevices.AnalogSensor.passive
static mp_obj_t iodevices_AnalogSensor_passive(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_type_device_get_data_blocking(self_in, PBDRV_LEGODEV_MODE_NXT_ANALOG__PASSIVE);
    self->active = false;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_passive_obj, iodevices_AnalogSensor_passive);

// dir(pybricks.iodevices.AnalogSensor)
static const mp_rom_map_elem_t iodevices_AnalogSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_voltage),    MP_ROM_PTR(&iodevices_AnalogSensor_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_resistance), MP_ROM_PTR(&iodevices_AnalogSensor_resistance_obj) },
    { MP_ROM_QSTR(MP_QSTR_active),     MP_ROM_PTR(&iodevices_AnalogSensor_active_obj)    },
    { MP_ROM_QSTR(MP_QSTR_passive),    MP_ROM_PTR(&iodevices_AnalogSensor_passive_obj)    },
};
static MP_DEFINE_CONST_DICT(iodevices_AnalogSensor_locals_dict, iodevices_AnalogSensor_locals_dict_table);

// type(pybricks.iodevices.AnalogSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_iodevices_AnalogSensor,
    MP_QSTR_AnalogSensor,
    MP_TYPE_FLAG_NONE,
    make_new, iodevices_AnalogSensor_make_new,
    locals_dict, &iodevices_AnalogSensor_locals_dict);

#endif // PYBRICKS_PY_IODEVICES && PYBRICKS_PY_EV3DEVICES
