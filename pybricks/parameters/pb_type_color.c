// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <inttypes.h>

#include <pbio/color.h>

#include <pybricks/parameters.h>

static const qstr qstr_empty = MP_QSTR__;

typedef struct _parameters_Color_obj_t {
    mp_obj_base_t base;
    qstr name;
    pbio_color_hsv_t hsv;
} parameters_Color_obj_t;

// FIXME: Drop leading underscore once the legacy pb_Color_NAME_obj are all removed
const parameters_Color_obj_t _pb_Color_RED_obj = {
    {&pb_type_Color},
    .name = MP_QSTR_RED,
    .hsv = {0, 100, 100}
};

void pb_type_Color_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    parameters_Color_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Color(%" PRIu16 ", %" PRIu8 ", %" PRIu8, self->hsv.h, self->hsv.s, self->hsv.v);
    if (self->name != qstr_empty) {
        mp_printf(print, ", '%q'", self->name);
    }
    mp_printf(print, ")");   
}

STATIC const mp_rom_map_elem_t pb_type_Color_table[] = {
    { MP_ROM_QSTR(MP_QSTR_RED),            MP_ROM_PTR(&_pb_Color_RED_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_type_Color_locals_dict, pb_type_Color_table);

const mp_obj_type_t pb_type_Color = {
    { &mp_type_type },
    .name = MP_QSTR_Color,
    .print = pb_type_Color_print,
    .unary_op = mp_generic_unary_op,
    .locals_dict = (mp_obj_dict_t *)&(pb_type_Color_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS
