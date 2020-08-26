// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <inttypes.h>

#include <pbio/color.h>

#include "py/objstr.h"

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

typedef struct _parameters_Color_obj_t {
    mp_obj_base_t base;
    mp_obj_t name;
    pbio_color_hsv_t hsv;
} parameters_Color_obj_t;

// FIXME: Drop leading underscore once the legacy pb_Color_NAME_obj are all removed
const parameters_Color_obj_t _pb_Color_RED_obj = {
    {&pb_type_Color},
    .name = MP_OBJ_NEW_QSTR(MP_QSTR_RED),
    .hsv = {0, 100, 100}
};

mp_obj_t parameters_Color_make_new_helper(uint16_t h, uint8_t s, uint8_t v, mp_obj_t name) {
    parameters_Color_obj_t *self = m_new_obj(parameters_Color_obj_t);
    self->base.type = &pb_type_Color;
    self->hsv.h = h;
    self->hsv.s = s;
    self->hsv.v = v;
    self->name = name;
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t parameters_Color_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(h),
        PB_ARG_DEFAULT_INT(s, 100),
        PB_ARG_DEFAULT_INT(v, 100),
        PB_ARG_DEFAULT_NONE(name));

    // Name must be None or a string
    if (name_in != mp_const_none && !mp_obj_is_qstr(name_in)) {
        pb_assert_type(name_in, &mp_type_str);
    }

    return parameters_Color_make_new_helper(pb_obj_get_hue(h_in), pb_obj_get_pct(s_in), pb_obj_get_pct(v_in), name_in);
}

void pb_type_Color_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    parameters_Color_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Color(%" PRIu16 ", %" PRIu8 ", %" PRIu8, self->hsv.h, self->hsv.s, self->hsv.v);
    if (self->name != mp_const_none) {
        if (MP_OBJ_IS_QSTR(self->name)) {
            mp_printf(print, ", '%q'", MP_OBJ_QSTR_VALUE(self->name));
        } else {
            mp_printf(print, ", '%s'", ((mp_obj_str_t *)MP_OBJ_TO_PTR(self->name))->data);
        }
    }
    mp_printf(print, ")");
}

STATIC void pb_type_Color_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    parameters_Color_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Return the requested object, read only
    if (dest[0] == MP_OBJ_NULL) {
        switch (attr)
        {
            case MP_QSTR_h:
                dest[0] = MP_OBJ_NEW_SMALL_INT(self->hsv.h);
                return;
            case MP_QSTR_s:
                dest[0] = MP_OBJ_NEW_SMALL_INT(self->hsv.s);
                return;
            case MP_QSTR_v:
                dest[0] = MP_OBJ_NEW_SMALL_INT(self->hsv.v);
                return;
            case MP_QSTR_name:
                dest[0] = self->name;
                return;
            default:
                break;
        }
    }

    // Not found, tell MicroPython to look at local_dict instead
    dest[1] = MP_OBJ_SENTINEL;
}


STATIC const mp_rom_map_elem_t pb_type_Color_table[] = {
    { MP_ROM_QSTR(MP_QSTR_RED),            MP_ROM_PTR(&_pb_Color_RED_obj)},
};
STATIC MP_DEFINE_CONST_DICT(pb_type_Color_locals_dict, pb_type_Color_table);

const mp_obj_type_t pb_type_Color = {
    { &mp_type_type },
    .name = MP_QSTR_Color,
    .attr = pb_type_Color_attr,
    .print = pb_type_Color_print,
    .unary_op = mp_generic_unary_op,
    .make_new = parameters_Color_make_new,
    .locals_dict = (mp_obj_dict_t *)&(pb_type_Color_locals_dict),
};

#endif // PYBRICKS_PY_PARAMETERS
