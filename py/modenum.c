// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include "modenum.h"

#include "py/obj.h"

void enum_class_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    pb_obj_enum_elem_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, qstr_str(self->base.type->name));
    mp_printf(print, ".");
    mp_printf(print, qstr_str(self->name));
}
