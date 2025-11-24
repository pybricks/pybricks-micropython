// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_IODEVICES_ANALOG_SENSOR

#include "py/mphal.h"
#include "py/smallint.h"

#include <pbio/int_math.h>
#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.iodevices.AnalogSensor class object
typedef struct _iodevices_AnalogSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
    bool active;
} iodevices_AnalogSensor_obj_t;

// pybricks.iodevices.AnalogSensor.__init__
static mp_obj_t iodevices_AnalogSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_FALSE(custom)
        );

    pb_module_tools_assert_blocking();

    // Get the port instance.
    iodevices_AnalogSensor_obj_t *self = mp_obj_malloc(iodevices_AnalogSensor_obj_t, type);
    pb_assert(pbio_port_get_port(pb_type_enum_get_value(port_in, &pb_enum_type_Port), &self->port));

    // Set the port mode to LEGO DCM or raw ADC mode.
    pbio_error_t err = pbio_port_set_mode(self->port, mp_obj_is_true(custom_in) ? PBIO_PORT_MODE_GPIO_ADC : PBIO_PORT_MODE_LEGO_DCM);
    if (err == PBIO_ERROR_AGAIN) {
        // If coming from a different mode, give port some time to get started.
        // This happens when the user has a custom device and decides to switch
        // back to LEGO mode. This should be rare, so we can afford to wait.
        mp_hal_delay_ms(1000);
        err = pbio_port_set_mode(self->port, PBIO_PORT_MODE_LEGO_DCM);
    }
    pb_assert(err);

    // Start as passive by default.
    uint32_t analog;
    self->active = false;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, self->active, &analog));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.iodevices.AnalogSensor.voltage
static mp_obj_t iodevices_AnalogSensor_voltage(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t voltage;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, self->active, &voltage));
    return mp_obj_new_int(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_voltage_obj, iodevices_AnalogSensor_voltage);

// pybricks.iodevices.AnalogSensor.resistance
static mp_obj_t iodevices_AnalogSensor_resistance(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t voltage;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, self->active, &voltage));

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
    self->active = true;
    uint32_t voltage;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, self->active, &voltage));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(iodevices_AnalogSensor_active_obj, iodevices_AnalogSensor_active);

// pybricks.iodevices.AnalogSensor.passive
static mp_obj_t iodevices_AnalogSensor_passive(mp_obj_t self_in) {
    iodevices_AnalogSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->active = false;
    uint32_t voltage;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, self->active, &voltage));
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

#endif // PYBRICKS_PY_IODEVICES_ANALOG_SENSOR
