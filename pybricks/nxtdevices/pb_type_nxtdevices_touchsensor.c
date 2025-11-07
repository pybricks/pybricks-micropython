
// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_NXTDEVICES

#include <pbio/port_interface.h>
#include <pbio/port_dcm.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// pybricks.nxtdevices.TouchSensor class object
typedef struct _nxtdevices_TouchSensor_obj_t {
    mp_obj_base_t base;
    pbio_port_t *port;
} nxtdevices_TouchSensor_obj_t;

// pybricks.nxtdevices.TouchSensor.pressed
static mp_obj_t nxtdevices_TouchSensor_pressed(mp_obj_t self_in) {
    nxtdevices_TouchSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t analog = 0;
    pb_assert(pbio_port_get_analog_value(self->port, LEGO_DEVICE_TYPE_ID_NXT_TOUCH_SENSOR, false, &analog));
    return mp_obj_new_bool(analog < 3000);
}
static MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_TouchSensor_pressed_obj, nxtdevices_TouchSensor_pressed);

// pybricks.nxtdevices.TouchSensor.__init__
static mp_obj_t nxtdevices_TouchSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    nxtdevices_TouchSensor_obj_t *self = mp_obj_malloc(nxtdevices_TouchSensor_obj_t, type);
    pbio_port_id_t port_id = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pb_assert(pbio_port_get_port(port_id, &self->port));

    // Measure once. This will assert that something else is not attached.
    mp_obj_t self_obj = MP_OBJ_FROM_PTR(self);
    nxtdevices_TouchSensor_pressed(self_obj);
    return self_obj;
}

// dir(pybricks.nxtdevices.TouchSensor)
static const mp_rom_map_elem_t nxtdevices_TouchSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pressed), MP_ROM_PTR(&nxtdevices_TouchSensor_pressed_obj) },
};
static MP_DEFINE_CONST_DICT(nxtdevices_TouchSensor_locals_dict, nxtdevices_TouchSensor_locals_dict_table);

// type(pybricks.nxtdevices.TouchSensor)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_nxtdevices_TouchSensor,
    MP_QSTR_TouchSensor,
    MP_TYPE_FLAG_NONE,
    make_new, nxtdevices_TouchSensor_make_new,
    locals_dict, &nxtdevices_TouchSensor_locals_dict);

#endif // PYBRICKS_PY_NXTDEVICES
