// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

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

// pybricks.nxtdevices.ViernierAdapter class object
typedef struct _nxtdevices_vernier_adapter_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
    mp_obj_t conversion;
} nxtdevices_vernier_adapter_obj_t;

// pybricks.nxtdevices.ViernierAdapter.__init__
static mp_obj_t nxtdevices_vernier_adapter_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port),
        PB_ARG_DEFAULT_NONE(conversion));

    pb_module_tools_assert_blocking();

    if (!mp_obj_is_callable(conversion_in)) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Get the port instance.
    nxtdevices_vernier_adapter_obj_t *self = mp_obj_malloc(nxtdevices_vernier_adapter_obj_t, type);
    pb_assert(pbio_port_get_port(pb_type_enum_get_value(port_in, &pb_enum_type_Port), &self->port));
    self->conversion = conversion_in;

    // This is detected as an NXT analog device, so can use standard mode.
    pb_device_set_lego_mode(self->port);

    // Start as passive by default.
    uint32_t voltage;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, false, &voltage));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.ViernierAdapter.voltage
static mp_obj_t nxtdevices_vernier_adapter_voltage(mp_obj_t self_in) {
    nxtdevices_vernier_adapter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t voltage;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, false, &voltage));
    return mp_obj_new_int(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_vernier_adapter_voltage_obj, nxtdevices_vernier_adapter_voltage);

// pybricks.nxtdevices.ViernierAdapter.value
static mp_obj_t nxtdevices_vernier_adapter_value(mp_obj_t self_in) {
    nxtdevices_vernier_adapter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t voltage;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_ANALOG, false, &voltage));
    return mp_call_function_1(self->conversion, mp_obj_new_int(voltage));
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_vernier_adapter_value_obj, nxtdevices_vernier_adapter_value);

// pybricks.nxtdevices.ViernierAdapter.conversion
static mp_obj_t nxtdevices_vernier_adapter_conversion(mp_obj_t self_in, mp_obj_t voltage_in) {
    nxtdevices_vernier_adapter_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_call_function_1(self->conversion, voltage_in);
}
static MP_DEFINE_CONST_FUN_OBJ_2(nxtdevices_vernier_adapter_conversion_obj, nxtdevices_vernier_adapter_conversion);

// dir(pybricks.nxtdevices.ViernierAdapter)
static const mp_rom_map_elem_t nxtdevices_vernier_adapter_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_voltage),    MP_ROM_PTR(&nxtdevices_vernier_adapter_voltage_obj)    },
    { MP_ROM_QSTR(MP_QSTR_value),      MP_ROM_PTR(&nxtdevices_vernier_adapter_value_obj)      },
    { MP_ROM_QSTR(MP_QSTR_conversion), MP_ROM_PTR(&nxtdevices_vernier_adapter_conversion_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_vernier_adapter_locals_dict, nxtdevices_vernier_adapter_locals_dict_table);

// type(pybricks.nxtdevices.ViernierAdapter)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_VernierAdapter,
    MP_QSTR_VernierAdapter,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_vernier_adapter_make_new,
    locals_dict, &nxtdevices_vernier_adapter_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
