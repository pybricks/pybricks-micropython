// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_type_enum.h>

void pb_type_enum_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    pb_obj_enum_member_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "%q.%q", self->base.type->name, self->name);
}

mp_int_t pb_type_enum_get_value(mp_obj_t obj, const mp_obj_type_t *type) {
    pb_assert_type(obj, type);
    pb_obj_enum_member_t *member = MP_OBJ_TO_PTR(obj);
    return member->value;
}
