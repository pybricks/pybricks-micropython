// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbio/battery.h>
#include <pbio/dcmotor.h>

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

// Class structure for Light
typedef struct _pupdevices_Light_obj_t {
    mp_obj_base_t base;
    pbio_dcmotor_t *dcmotor;
} pupdevices_Light_obj_t;

// pybricks.pupdevices.Light.__init__
static mp_obj_t pupdevices_Light_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_Light_obj_t *self = mp_obj_malloc(pupdevices_Light_obj_t, type);

    pbio_port_id_t port = pb_type_enum_get_value(port_in, &pb_enum_type_Port);
    pbdrv_legodev_dev_t *legodev;
    pbdrv_legodev_type_id_t id = PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT;
    pb_assert(pbdrv_legodev_get_device(port, &id, &legodev));

    // Get and initialize DC Motor
    pb_assert(pbio_dcmotor_get_dcmotor(legodev, &self->dcmotor));
    pb_assert(pbio_dcmotor_setup(self->dcmotor, PBDRV_LEGODEV_TYPE_ID_LPF2_LIGHT, PBIO_DIRECTION_CLOCKWISE));

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.Light.on
static mp_obj_t pupdevices_Light_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_Light_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    // Set the brightness
    int32_t voltage = pbio_battery_get_voltage_from_duty_pct(pb_obj_get_pct(brightness_in));
    pb_assert(pbio_dcmotor_user_command(self->dcmotor, false, voltage));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_Light_on_obj, 1, pupdevices_Light_on);

// pybricks.pupdevices.Light.off
static mp_obj_t pupdevices_Light_off(mp_obj_t self_in) {
    pupdevices_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_assert(pbio_dcmotor_user_command(self->dcmotor, true, 0));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pupdevices_Light_off_obj, pupdevices_Light_off);

// dir(pybricks.pupdevices.Light)
static const mp_rom_map_elem_t pupdevices_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on),      MP_ROM_PTR(&pupdevices_Light_on_obj)           },
    { MP_ROM_QSTR(MP_QSTR_off),     MP_ROM_PTR(&pupdevices_Light_off_obj)          },
};
static MP_DEFINE_CONST_DICT(pupdevices_Light_locals_dict, pupdevices_Light_locals_dict_table);

// type(pybricks.pupdevices.Light)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_pupdevices_Light,
    MP_QSTR_Light,
    MP_TYPE_FLAG_NONE,
    make_new, pupdevices_Light_make_new,
    locals_dict, &pupdevices_Light_locals_dict);

#endif // PYBRICKS_PY_PUPDEVICES
