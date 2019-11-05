// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pberror.h>
#include <pbio/iodev.h>
#include <pbio/ev3device.h>

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objtype.h"

#include "pbobj.h"
#include "pbkwarg.h"

#include "modparameters.h"

// pybricks.nxtdevices.UltrasonicSensor class object
typedef struct _nxtdevices_UltrasonicSensor_obj_t {
    mp_obj_base_t base;
    pbio_ev3iodev_t *iodev;
} nxtdevices_UltrasonicSensor_obj_t;

// pybricks.nxtdevices.UltrasonicSensor.__init__
STATIC mp_obj_t nxtdevices_UltrasonicSensor_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port)
    );

    nxtdevices_UltrasonicSensor_obj_t *self = m_new_obj(nxtdevices_UltrasonicSensor_obj_t);
    self->base.type = (mp_obj_type_t*) type;

    mp_int_t port_num = enum_get_value_maybe(port, &pb_enum_type_Port);
    pb_assert(ev3device_get_device(&self->iodev, PBIO_IODEV_TYPE_ID_NXT_ULTRASONIC_SENSOR, port_num));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.nxtdevices.UltrasonicSensor.__str__
STATIC void nxtdevices_UltrasonicSensor_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(MP_QSTR_UltrasonicSensor));
    mp_printf(print, " on Port.S%c",  self->iodev->port);
}

// pybricks.nxtdevices.UltrasonicSensor.distance
STATIC mp_obj_t nxtdevices_UltrasonicSensor_distance(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_FALSE(silent)
    );
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    uint8_t distance;
    if (mp_obj_is_true(silent)) {
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__SI_CM, &distance));
    }
    else {
        pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__DIST_CM, &distance));
    }
    return mp_obj_new_int(distance * 10);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(nxtdevices_UltrasonicSensor_distance_obj, 0, nxtdevices_UltrasonicSensor_distance);

// pybricks.nxtdevices.UltrasonicSensor.presence
STATIC mp_obj_t nxtdevices_UltrasonicSensor_presence(mp_obj_t self_in) {
    nxtdevices_UltrasonicSensor_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t presence;
    pb_assert(ev3device_get_values_at_mode(self->iodev, PBIO_IODEV_MODE_NXT_ULTRASONIC_SENSOR__LISTEN, &presence));
    return mp_obj_new_bool(presence);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(nxtdevices_UltrasonicSensor_presence_obj, nxtdevices_UltrasonicSensor_presence);

// dir(pybricks.nxtdevices.UltrasonicSensor)
STATIC const mp_rom_map_elem_t nxtdevices_UltrasonicSensor_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_distance), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_presence), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_presence_obj) },
};
STATIC MP_DEFINE_CONST_DICT(nxtdevices_UltrasonicSensor_locals_dict, nxtdevices_UltrasonicSensor_locals_dict_table);

// type(pybricks.nxtdevices.UltrasonicSensor)
STATIC const mp_obj_type_t nxtdevices_UltrasonicSensor_type = {
    { &mp_type_type },
    .name = MP_QSTR_UltrasonicSensor,
    .print = nxtdevices_UltrasonicSensor_print,
    .make_new = nxtdevices_UltrasonicSensor_make_new,
    .locals_dict = (mp_obj_dict_t*)&nxtdevices_UltrasonicSensor_locals_dict,
};

// dir(pybricks.nxtdevices)
STATIC const mp_rom_map_elem_t nxtdevices_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),         MP_ROM_QSTR(MP_QSTR_nxtdevices)              },
#ifdef PBDRV_CONFIG_HUB_NXTBRICK
    { MP_ROM_QSTR(MP_QSTR_Motor),            MP_ROM_PTR(&motor_Motor_type)                },
#endif
    { MP_ROM_QSTR(MP_QSTR_UltrasonicSensor), MP_ROM_PTR(&nxtdevices_UltrasonicSensor_type)},
};

STATIC MP_DEFINE_CONST_DICT(pb_module_nxtdevices_globals, nxtdevices_globals_table);

const mp_obj_module_t pb_module_nxtdevices = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_nxtdevices_globals,
};

