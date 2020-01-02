// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include "py/obj.h"
#include "py/runtime.h"

#include "pb_type_enum.h"
#include "pberror.h"

void pb_type_enum_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    pb_obj_enum_member_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(self->base.type->name));
    mp_printf(print, ".");
    mp_printf(print, qstr_str(self->name));
}

mp_int_t pb_type_enum_get_value(mp_obj_t obj, const mp_obj_type_t *type) {
    pb_obj_enum_member_t *member = obj;

    if (!mp_obj_is_type(obj, type)) {
        if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
            mp_raise_TypeError("requires enum");
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                "can't convert %s to %s", mp_obj_get_type_str(obj), qstr_str(type->name)));
        }
    }
    return member->value;
}
