// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#include <pbio/color.h>

#include "py/objstr.h"

#include <pybricks/parameters.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

const pb_type_Color_obj_t pb_Color_RED_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_RED, 100, 100}
};

const pb_type_Color_obj_t pb_Color_BROWN_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_ORANGE, 100, 50}
};

const pb_type_Color_obj_t pb_Color_ORANGE_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_ORANGE, 100, 100}
};

const pb_type_Color_obj_t pb_Color_YELLOW_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_YELLOW, 100, 100}
};

const pb_type_Color_obj_t pb_Color_GREEN_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_GREEN, 100, 100}
};

const pb_type_Color_obj_t pb_Color_CYAN_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_CYAN, 100, 100}
};

const pb_type_Color_obj_t pb_Color_BLUE_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_BLUE, 100, 100}
};

const pb_type_Color_obj_t pb_Color_VIOLET_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_VIOLET, 100, 100}
};

const pb_type_Color_obj_t pb_Color_MAGENTA_obj = {
    {&pb_type_Color},
    .hsv = {PBIO_COLOR_HUE_MAGENTA, 100, 100}
};

const pb_type_Color_obj_t pb_Color_NONE_obj = {
    {&pb_type_Color},
    .hsv = {0, 0, 0}
};

const pb_type_Color_obj_t pb_Color_BLACK_obj = {
    {&pb_type_Color},
    .hsv = {0, 0, 10}
};

const pb_type_Color_obj_t pb_Color_GRAY_obj = {
    {&pb_type_Color},
    .hsv = {0, 0, 50}
};

const pb_type_Color_obj_t pb_Color_WHITE_obj = {
    {&pb_type_Color},
    .hsv = {0, 0, 100}
};

/**
 * Gets the pointer to the hsv type from a Color type.
 *
 * If @p obj is not a Color type, a TypeError is raised.
 * If @p obj is None, it is treated as Color.BLACK.
 *
 * @param obj [in]  A MicroPython object of pb_type_Color
 * @return          Pointer to hsv structure
 */
const pbio_color_hsv_t *pb_type_Color_get_hsv(mp_obj_t obj) {

    // For none, return HSV (0, 0, 0), corresponding to off
    if (obj == mp_const_none) {
        return &pb_Color_NONE_obj.hsv;
    }

    // Assert type and extract hsv
    pb_assert_type(obj, &pb_type_Color);
    return &((pb_type_Color_obj_t *)obj)->hsv;
}

pb_type_Color_obj_t *pb_type_Color_new_empty(void) {
    pb_type_Color_obj_t *color = m_new_obj(pb_type_Color_obj_t);
    color->base.type = &pb_type_Color;
    return color;
}

static mp_obj_t pb_type_Color_make_new_helper(mp_int_t h, mp_int_t s, mp_int_t v) {
    pb_type_Color_obj_t *self = pb_type_Color_new_empty();

    // Bind h to 0--360
    h = h % 360;
    self->hsv.h = h < 0 ? h + 360 : h;

    // Bind s to 0--100
    s = s < 0 ? 0 : s;
    self->hsv.s = s > 100 ? 100 : s;

    // Bind v to 0--100
    v = v < 0 ? 0 : v;
    self->hsv.v = v > 100 ? 100 : v;

    return MP_OBJ_FROM_PTR(self);
}

// Create and reset dict to hold default colors and those added by user
STATIC mp_obj_dict_t *colors;

void pb_type_Color_reset(void) {
    // Set default contents of Color class
    colors = mp_obj_new_dict(0);
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_RED),     MP_OBJ_FROM_PTR(&pb_Color_RED_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_BROWN),   MP_OBJ_FROM_PTR(&pb_Color_BROWN_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_ORANGE),  MP_OBJ_FROM_PTR(&pb_Color_ORANGE_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_YELLOW),  MP_OBJ_FROM_PTR(&pb_Color_YELLOW_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_GREEN),   MP_OBJ_FROM_PTR(&pb_Color_GREEN_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_CYAN),    MP_OBJ_FROM_PTR(&pb_Color_CYAN_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_BLUE),    MP_OBJ_FROM_PTR(&pb_Color_BLUE_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_MAGENTA), MP_OBJ_FROM_PTR(&pb_Color_MAGENTA_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_VIOLET),  MP_OBJ_FROM_PTR(&pb_Color_VIOLET_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_BLACK),   MP_OBJ_FROM_PTR(&pb_Color_BLACK_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_GRAY),    MP_OBJ_FROM_PTR(&pb_Color_GRAY_obj));
    mp_obj_dict_store(MP_OBJ_FROM_PTR(colors), MP_ROM_QSTR(MP_QSTR_WHITE),   MP_OBJ_FROM_PTR(&pb_Color_WHITE_obj));
}

void pb_type_Color_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {

    // If we're the class itself, use dict printer
    if (MP_OBJ_TO_PTR(self_in) == &pb_type_Color_obj) {
        mp_type_dict.print(print, colors, kind);
        return;
    }

    // Otherwise, print name of color, if available
    mp_map_elem_t *color_elems = colors->map.table;
    for (size_t i = 0; i < colors->map.used; i++) {
        mp_map_elem_t *element = &color_elems[i];
        if (self_in == element->value && MP_OBJ_IS_QSTR(element->key)) {
            mp_printf(print, "Color.%q", MP_OBJ_QSTR_VALUE(element->key));
            return;
        }
    }

    // Otherwise, print hsv representation that can be evaluated
    pb_type_Color_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Color(h=%u, s=%u, v=%u)", self->hsv.h, self->hsv.s, self->hsv.v);
}

STATIC mp_obj_t pb_type_Color_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value) {

    // If we're a Color instance, there is no subscr
    if (MP_OBJ_TO_PTR(self_in) != &pb_type_Color_obj) {
        return MP_OBJ_NULL;
    }

    // If user wants to store, ensure they store color
    if (value != MP_OBJ_SENTINEL && value != MP_OBJ_NULL) {
        pb_assert_type(value, &pb_type_Color);
    }

    // Treat it like a dictionary
    return mp_type_dict.subscr(colors, index, value);
}

STATIC mp_obj_t pb_type_Color_getiter(mp_obj_t self_in, mp_obj_iter_buf_t *iter_buf) {

    // If we're a Color instance, there is no getiter
    if (MP_OBJ_TO_PTR(self_in) != &pb_type_Color_obj) {
        return MP_OBJ_NULL;
    }

    // Treat it like a dictionary
    return mp_type_dict.getiter(colors, iter_buf);
}

STATIC void pb_type_Color_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {

    // If we're a Color instance, just check h, s, v attributes
    if (MP_OBJ_TO_PTR(self_in) != &pb_type_Color_obj) {

        // Colors are immutable, so return if they try to write or delete.
        if (dest[0] != MP_OBJ_NULL) {
            return;
        }

        // Get the color
        pb_type_Color_obj_t *self = MP_OBJ_TO_PTR(self_in);

        // Return h, s, or v as requested
        switch (attr) {
            case MP_QSTR_h:
                dest[0] = MP_OBJ_NEW_SMALL_INT(self->hsv.h);
                return;
            case MP_QSTR_s:
                dest[0] = MP_OBJ_NEW_SMALL_INT(self->hsv.s);
                return;
            case MP_QSTR_v:
                dest[0] = MP_OBJ_NEW_SMALL_INT(self->hsv.v);
                return;
            default:
                break;
        }
        return;
    }

    // If we're here, we are the Color class, so treat as dict, converting the
    // attr and dest to the expected format for subscr().
    mp_obj_t arg = (dest[0] == MP_OBJ_NULL) ? MP_OBJ_SENTINEL : // load else
        ((dest[1] == MP_OBJ_NULL) ? MP_OBJ_NULL : dest[1]); // delete else write

    // Read from dictionary. For subscr(), none means success. For attr, null is success.
    dest[0] = pb_type_Color_subscr(self_in, MP_OBJ_NEW_QSTR(attr), arg);
    if (dest[0] == mp_const_none) {
        dest[0] = MP_OBJ_NULL;
    }
}

STATIC mp_obj_t pb_type_Color_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {

    pb_type_Color_obj_t *self = MP_OBJ_TO_PTR(lhs_in);

    switch (op) {
        case MP_BINARY_OP_EQUAL:
            // Two colors are equal if their h, s, and v are equal
            if (!mp_obj_is_type(rhs_in, &pb_type_Color)) {
                return mp_const_false;
            }
            pb_type_Color_obj_t *other = MP_OBJ_TO_PTR(rhs_in);
            if (self->hsv.h == other->hsv.h &&
                self->hsv.s == other->hsv.s &&
                self->hsv.v == other->hsv.v) {
                return mp_const_true;
            } else {
                return mp_const_false;
            }
        case MP_BINARY_OP_LSHIFT:
            // lshift is negative rshift, so negate and fall through to rshift
            rhs_in = mp_obj_new_int(-pb_obj_get_int(rhs_in));
        case MP_BINARY_OP_RSHIFT:
            // Color shifting shifts the hue
            return pb_type_Color_make_new_helper(
                self->hsv.h + pb_obj_get_int(rhs_in),
                self->hsv.s,
                self->hsv.v);
        case MP_BINARY_OP_MULTIPLY:
        // For both A*c and c*A, MicroPython calls c the rhs_in,
        // so we can just fall through and treat both the same here.
        case MP_BINARY_OP_REVERSE_MULTIPLY: {
            // Multiply multiplies the value.
            #if MICROPY_PY_BUILTINS_FLOAT
            mp_int_t value = (mp_int_t)(mp_obj_get_float(rhs_in) * self->hsv.v);
            #else
            mp_int_t value = mp_obj_get_int(rhs_in) * self->hsv.v;
            #endif
            return pb_type_Color_make_new_helper(
                self->hsv.h,
                self->hsv.s,
                value);
        }
        case MP_BINARY_OP_FLOOR_DIVIDE:
        // Fall through since both floor and true divide eventually
        // truncate value to integer, which is stored in the hsv type.
        case MP_BINARY_OP_TRUE_DIVIDE: {
            // Divide divides the value
            #if MICROPY_PY_BUILTINS_FLOAT
            mp_int_t value = (mp_int_t)(self->hsv.v / mp_obj_get_float(rhs_in));
            #else
            mp_int_t value = self->hsv.v / mp_obj_get_int(rhs_in);
            #endif
            return pb_type_Color_make_new_helper(
                self->hsv.h,
                self->hsv.s,
                value);
        }
        default:
            // Other operations not supported
            return MP_OBJ_NULL;
    }
}

// pybricks.parameters.Color
STATIC mp_obj_t pb_type_Color_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(h),
        PB_ARG_DEFAULT_INT(s, 100),
        PB_ARG_DEFAULT_INT(v, 100));

    return pb_type_Color_make_new_helper(pb_obj_get_int(h_in), pb_obj_get_int(s_in), pb_obj_get_int(v_in));
}

const mp_obj_type_t pb_type_Color = {
    { &mp_type_type },
    .name = MP_QSTR_Color,
    .call = pb_type_Color_call,
    .attr = pb_type_Color_attr,
    .print = pb_type_Color_print,
    .unary_op = mp_generic_unary_op,
    .binary_op = pb_type_Color_binary_op,
    .subscr = pb_type_Color_subscr,
    .getiter = pb_type_Color_getiter,
};

// We expose an instance instead of the type. This lets us provide class
// attributes via the attribute handler for more flexibility.
const mp_obj_base_t pb_type_Color_obj = {
    &pb_type_Color
};

#endif // PYBRICKS_PY_PARAMETERS
