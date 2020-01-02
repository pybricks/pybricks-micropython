// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include "modenum.h"
#include "pberror.h"

#include "py/obj.h"
#include "py/runtime.h"

void enum_class_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    pb_obj_enum_elem_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(self->base.type->name));
    mp_printf(print, ".");
    mp_printf(print, qstr_str(self->name));
}

mp_int_t enum_get_value_maybe(mp_obj_t enum_elem, const mp_obj_type_t *valid_type) {
    pb_obj_enum_elem_t *elem = enum_elem;

    if (!mp_obj_is_type(enum_elem, valid_type)) {
        if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
            mp_raise_TypeError("requires enum");
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                "can't convert %s to %s", mp_obj_get_type_str(enum_elem), qstr_str(valid_type->name)));
        }
    }
    return elem->value;
}
