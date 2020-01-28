// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/control.h>
#include <pbio/button.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "modcontrol.h"
#include "pbdevice.h"

#include "modcontrol.h"
#include "modparameters.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.builtins.Light class object
typedef struct _control_Light_obj_t {
    mp_obj_base_t base;
    pbdevice_t *pbdev;
} control_Light_obj_t;

// pybricks.builtins.Light.on
STATIC mp_obj_t control_Light_on(mp_obj_t self_in) {
    pb_assert(PBIO_ERROR_NOT_SUPPORTED);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(control_Light_on_obj, control_Light_on);

// pybricks.builtins.ColorLight.on
STATIC mp_obj_t control_ColorLight_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse arguments
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        control_Light_obj_t, self,
        PB_ARG_REQUIRED(color),
        PB_ARG_DEFAULT_INT(brightness, 100)
    );

    if (color == mp_const_none) {
        color = MP_OBJ_FROM_PTR(&pb_const_black);
    }

    pbio_control_color_t color_id = pb_type_enum_get_value(color, &pb_enum_type_Color);

    mp_int_t bright = pb_obj_get_int(brightness);
    bright = bright < 0 ? 0 : bright > 100 ? 100: bright;

    // TODO: Brightness control is not yet implemented
    if (bright != 100) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    if (!self->pbdev) {
        // No external device, so assume command is for the internal control
        pb_assert(pbio_control_on(PBIO_PORT_SELF, color_id));
    }
    else {
        pbdevice_color_control_on(self->pbdev, color_id);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(control_ColorLight_on_obj, 0, control_ColorLight_on);

// pybricks.builtins.Light.off
// pybricks.builtins.LightArray.off
// pybricks.builtins.ColorLight.off
STATIC mp_obj_t control_Light_off(mp_obj_t self_in) {
    control_Light_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Turn the control off, using the command specific to the device. 
    if (!self->pbdev) {
        // No external device, so assume command is for the internal control
        pb_assert(pbio_control_off(PBIO_PORT_SELF));
    }
    else {
        pbdevice_color_control_on(self->pbdev, PBIO_LIGHT_COLOR_NONE);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(control_Light_off_obj, control_Light_off);

// dir(pybricks.builtins.Light)
STATIC const mp_rom_map_elem_t control_Light_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&control_Light_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&control_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(control_Light_locals_dict, control_Light_locals_dict_table);

// type(pybricks.builtins.Light)
const mp_obj_type_t control_Light_type = {
    { &mp_type_type },
    .name = MP_QSTR_Light,
    .locals_dict = (mp_obj_dict_t*)&control_Light_locals_dict,
};

// dir(pybricks.builtins.ColorLight)
STATIC const mp_rom_map_elem_t control_ColorLight_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on   ), MP_ROM_PTR(&control_ColorLight_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off  ), MP_ROM_PTR(&control_Light_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(control_ColorLight_locals_dict, control_ColorLight_locals_dict_table);

// type(pybricks.builtins.ColorLight)
const mp_obj_type_t control_ColorLight_type = {
    { &mp_type_type },
    .name = MP_QSTR_ColorLight,
    .locals_dict = (mp_obj_dict_t*)&control_ColorLight_locals_dict,
};

mp_obj_t control_Light_obj_make_new(pbdevice_t *pbdev, const mp_obj_type_t *type) {
    // Create new control instance
    control_Light_obj_t *control = m_new_obj(control_Light_obj_t);
    // Set type and iodev
    control->base.type = type;
    control->pbdev = pbdev;
    return control;
}
