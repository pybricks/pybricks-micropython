// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for Light
typedef struct _pupdevices_Light_obj_t {
    mp_obj_base_t base;
    pbio_iodev_t *iodev;
} pupdevices_Light_obj_t;

// pybricks.pupdevices.Light.__init__
STATIC mp_obj_t pupdevices_Light_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_Light_obj_t *self = mp_obj_malloc(pupdevices_Light_obj_t, type);

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);

    // Get iodevices
    self->iodev = pb_pup_device_get_device(port, PBIO_IODEV_TYPE_ID_LPF2_LIGHT);

    return MP_OBJ_FROM_PTR(self);
}

STATIC void set_power(pbio_iodev_t *iodev, int32_t duty) {

    // FIXME: this should be a callback function on a port instance rather
    // than poking the motor driver directly. The current implementation
    // is only valid on Powered Up platforms and it assumes that motor driver
    // id corresponds to the port.

    #ifdef PBDRV_CONFIG_FIRST_MOTOR_PORT
    pbdrv_motor_driver_dev_t *motor_driver;
    pb_assert(pbdrv_motor_driver_get_dev(iodev->port - PBDRV_CONFIG_FIRST_MOTOR_PORT, &motor_driver));

    // Apply duty cycle in reverse to activate power
    pb_assert(pbdrv_motor_driver_set_duty_cycle(motor_driver, -PBDRV_MOTOR_DRIVER_MAX_DUTY * duty / 100));
    #endif
}

// pybricks.pupdevices.Light.on
STATIC mp_obj_t pupdevices_Light_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_Light_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    // Set the brightness
    set_power(self->iodev, pb_obj_get_pct(brightness_in));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_Light_on_obj, 1, pupdevices_Light_on);

// pybricks.pupdevices.Light.off
STATIC mp_obj_t pupdevices_Light_off(mp_obj_t self_in) {
    pupdevices_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);
    set_power(self->iodev, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_Light_off_obj, pupdevices_Light_off);

// dir(pybricks.pupdevices.Light)
STATIC const mp_rom_map_elem_t pupdevices_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on),      MP_ROM_PTR(&pupdevices_Light_on_obj)           },
    { MP_ROM_QSTR(MP_QSTR_off),     MP_ROM_PTR(&pupdevices_Light_off_obj)          },
};
STATIC MP_DEFINE_CONST_DICT(pupdevices_Light_locals_dict, pupdevices_Light_locals_dict_table);

// type(pybricks.pupdevices.Light)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_Light,
    MP_QSTR_Light,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_Light_make_new,
    locals_dict, &pupdevices_Light_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
