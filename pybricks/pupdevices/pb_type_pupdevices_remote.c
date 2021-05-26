// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PUPDEVICES

#include <pbsys/light.h>
#include <pbio/util.h>

#include "py/runtime.h"
#include "py/obj.h"

#include <pbio/button.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/common.h>

STATIC pbio_error_t remote_button_is_pressed(pbio_button_flags_t *pressed) {
    // TODO: Return last known remote button state
    *pressed = 512;
    return PBIO_SUCCESS;
}

typedef struct _pb_type_pupdevices_Remote_obj_t {
    mp_obj_base_t base;
    mp_obj_t buttons;
} pb_type_pupdevices_Remote_obj_t;

STATIC const pb_obj_enum_member_t *remote_buttons[] = {
    &pb_Button_LEFT_MINUS_obj,
    &pb_Button_RIGHT_MINUS_obj,
    &pb_Button_LEFT_obj,
    &pb_Button_CENTER_obj,
    &pb_Button_RIGHT_obj,
    &pb_Button_LEFT_PLUS_obj,
    &pb_Button_RIGHT_PLUS_obj
};

STATIC mp_obj_t pb_type_pupdevices_Remote_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_NONE(address),
        PB_ARG_DEFAULT_INT(timeout, 10000));

    pb_type_pupdevices_Remote_obj_t *self = m_new_obj(pb_type_pupdevices_Remote_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    (void)address_in;
    (void)timeout_in;

    self->buttons = pb_type_Keypad_obj_new(PBIO_ARRAY_SIZE(remote_buttons), remote_buttons, remote_button_is_pressed);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t pb_type_pupdevices_Remote_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_ATTRIBUTE_OFFSET(pb_type_pupdevices_Remote_obj_t, buttons) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_pupdevices_Remote_locals_dict, pb_type_pupdevices_Remote_locals_dict_table);

const mp_obj_type_t pb_type_pupdevices_Remote = {
    { &mp_type_type },
    .name = PYBRICKS_HUB_CLASS_NAME,
    .make_new = pb_type_pupdevices_Remote_make_new,
    .locals_dict = (mp_obj_dict_t *)&pb_type_pupdevices_Remote_locals_dict,
};

#endif // PYBRICKS_PY_PUPDEVICES
