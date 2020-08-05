// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include "py/mphal.h"

#include <pbdrv/ioport.h>

#include <pbio/button.h>
#include <pbio/color.h>
#include <pbio/iodev.h>
#include <pbio/light.h>
#include <pbio/math.h>

#include "py/obj.h"
#include "py/runtime.h"


#include "common/common.h"
#include "common/common_motors.h"
#include "parameters/parameters.h"

#include "util_pb/pb_color_map.h"
#include "util_pb/pb_device.h"
#include "util_pb/pb_error.h"
#include "util_mp/pb_kwarg_helper.h"
#include "util_mp/pb_obj_helper.h"

// Class structure for Light
typedef struct _pupdevices_Light_obj_t {
    mp_obj_base_t base;
    pb_device_t *pbdev;
} pupdevices_Light_obj_t;

// pybricks.pupdevices.Light.__init__
STATIC mp_obj_t pupdevices_Light_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(port));

    pupdevices_Light_obj_t *self = m_new_obj(pupdevices_Light_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_int_t port_num = pb_type_enum_get_value(port, &pb_enum_type_Port);

    // Get iodevices
    self->pbdev = pb_device_get_device(port_num, PBIO_IODEV_TYPE_ID_LPF2_LIGHT);

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.pupdevices.Light.on
STATIC mp_obj_t pupdevices_Light_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pupdevices_Light_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    // Set the brightness
    pb_device_set_power_supply(self->pbdev, pb_obj_get_int(brightness));

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pupdevices_Light_on_obj, 1, pupdevices_Light_on);

// pybricks.pupdevices.Light.off
STATIC mp_obj_t pupdevices_Light_off(mp_obj_t self_in) {
    pupdevices_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pb_device_set_power_supply(self->pbdev, 0);
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
const mp_obj_type_t pb_type_pupdevices_Light = {
    { &mp_type_type },
    .name = MP_QSTR_Light,
    .make_new = pupdevices_Light_make_new,
    .locals_dict = (mp_obj_dict_t *)&pupdevices_Light_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
