// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include "py/mpconfig.h"

#include "py/mphal.h"
#include "py/runtime.h"

#include "pbobj.h"
#include "pbkwarg.h"
#include "modparameters.h"

#include "py/objtype.h"

#include <pbio/iodev.h>
#include <pbio/ev3device.h>
#include <pberror.h>

// pybricks.customdevices.AnalogSensor class object
typedef struct _customdevices_AnalogSensor_obj_t {
    mp_obj_base_t base;
    bool active;
    pbio_ev3iodev_t *iodev;
} customdevices_AnalogSensor_obj_t;

// pybricks.customdevices.AnalogSensor.__init__
STATIC mp_obj_t customdevices_AnalogSensor_make_new(const mp_obj_type_t *otype, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_TRUE(verify_type)
    );
    customdevices_AnalogSensor_obj_t *self = m_new_obj(customdevices_AnalogSensor_obj_t);
    self->base.type = (mp_obj_type_t*) otype;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);

    pbio_iodev_type_id_t id = mp_obj_is_true(verify_type) ? PBIO_IODEV_TYPE_ID_NXT_ANALOG : PBIO_IODEV_TYPE_ID_CUSTOM_ANALOG;

    pbio_error_t err;
    while ((err = ev3device_get_device(&self->iodev, id, port_num)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(1000);
    }
    pb_assert(err);

    // Initialize NXT sensors to passive state
    int32_t voltage;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.customdevices.AnalogSensor.voltage
STATIC mp_obj_t customdevices_AnalogSensor_voltage(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pb_assert(ev3device_get_values_at_mode(self->iodev, mode, &voltage));
    return mp_obj_new_int(voltage);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_voltage_obj, customdevices_AnalogSensor_voltage);

// pybricks.customdevices.AnalogSensor.resistance
STATIC mp_obj_t customdevices_AnalogSensor_resistance(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    uint8_t mode = self->active ? PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE : PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE;
    pb_assert(ev3device_get_values_at_mode(self->iodev, mode, &voltage));

    // Open terminal/infinite resistance, return infinite resistance
    const int32_t vmax = 4972;
    if (voltage >= vmax) {
        return mp_obj_new_int(MP_SSIZE_MAX);
    }
    // Return as if a pure voltage divider between load and 10K internal resistor
    return mp_obj_new_int((10000*voltage)/(vmax-voltage));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_resistance_obj, customdevices_AnalogSensor_resistance);

// pybricks.customdevices.AnalogSensor.active
STATIC mp_obj_t customdevices_AnalogSensor_active(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__ACTIVE, &voltage));
    self->active = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_active_obj, customdevices_AnalogSensor_active);

// pybricks.customdevices.AnalogSensor.passive
STATIC mp_obj_t customdevices_AnalogSensor_passive(mp_obj_t self_in) {
    customdevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t voltage;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ANALOG__PASSIVE, &voltage));
    self->active = false;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(customdevices_AnalogSensor_passive_obj, customdevices_AnalogSensor_passive);

// dir(pybricks.customdevices.AnalogSensor)
STATIC const mp_rom_map_elem_t customdevices_AnalogSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_voltage),    MP_ROM_PTR(&customdevices_AnalogSensor_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_resistance), MP_ROM_PTR(&customdevices_AnalogSensor_resistance_obj) },
    { MP_ROM_QSTR(MP_QSTR_active),     MP_ROM_PTR(&customdevices_AnalogSensor_active_obj )    },
    { MP_ROM_QSTR(MP_QSTR_passive),    MP_ROM_PTR(&customdevices_AnalogSensor_passive_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(customdevices_AnalogSensor_locals_dict, customdevices_AnalogSensor_locals_dict_table);

// type(pybricks.customdevices.AnalogSensor)
STATIC const mp_obj_type_t customdevices_AnalogSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_AnalogSensor,
    .make_new = customdevices_AnalogSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&customdevices_AnalogSensor_locals_dict,
};

// dir(pybricks.customdevices)
STATIC const mp_rom_map_elem_t customdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_customdevices)              },
    { MP_ROM_QSTR(MP_QSTR_AnalogSensor),     MP_ROM_PTR(&customdevices_AnalogSensor_type)    },
};

STATIC MP_DEFINE_CONST_DICT(pb_module_customdevices_globals, customdevices_globals_table);
const mp_obj_module_t pb_module_customdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_customdevices_globals,
};
