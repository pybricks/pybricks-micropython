// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#if PYBRICKS_PY_PARAMETERS_ICON

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/parameters.h>
#include <pybricks/geometry.h>

#include <pybricks/util_pb/pb_error.h>

// Maps icons as integers
static uint32_t get_bitmap(qstr attr) {
    switch (attr)
    {
        case MP_QSTR_UP:
            return (0b00100 << 20) |  // ⬛⬛⬜⬛⬛
                   (0b01110 << 15) |  // ⬛⬜⬜⬜⬛
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b01110 << 05) |  // ⬛⬜⬜⬜⬛
                   (0b01110 << 00);   // ⬛⬜⬜⬜⬛
        case MP_QSTR_DOWN:
            return (0b01110 << 20) |  // ⬛⬜⬜⬜⬛
                   (0b01110 << 15) |  // ⬛⬜⬜⬜⬛
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b01110 << 05) |  // ⬛⬜⬜⬜⬛
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_LEFT:
            return (0b00100 << 20) |  // ⬛⬛⬜⬛⬛
                   (0b01111 << 15) |  // ⬛⬜⬜⬜⬜
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b01111 << 05) |  // ⬛⬜⬜⬜⬜
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_RIGHT:
            return (0b00100 << 20) |  // ⬛⬛⬜⬛⬛
                   (0b11110 << 15) |  // ⬜⬜⬜⬜⬛
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b11110 << 05) |  // ⬜⬜⬜⬜⬛
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_ARROW_RIGHT_UP:
            return (0b00111 << 20) |  // ⬛⬛⬜⬜⬜
                   (0b00011 << 15) |  // ⬛⬛⬛⬜⬜
                   (0b00101 << 10) |  // ⬛⬛⬜⬛⬜
                   (0b01000 << 05) |  // ⬛⬜⬛⬛⬛
                   (0b10000 << 00);   // ⬜⬛⬛⬛⬛
        case MP_QSTR_ARROW_RIGHT_DOWN:
            return (0b10000 << 20) |  // ⬜⬛⬛⬛⬛
                   (0b01000 << 15) |  // ⬛⬜⬛⬛⬛
                   (0b00101 << 10) |  // ⬛⬛⬜⬛⬜
                   (0b00011 << 05) |  // ⬛⬛⬛⬜⬜
                   (0b00111 << 00);   // ⬛⬛⬜⬜⬜
        case MP_QSTR_ARROW_LEFT_UP:
            return (0b11100 << 20) |  // ⬜⬜⬜⬛⬛
                   (0b11000 << 15) |  // ⬜⬜⬛⬛⬛
                   (0b10100 << 10) |  // ⬜⬛⬜⬛⬛
                   (0b00010 << 05) |  // ⬛⬛⬛⬜⬛
                   (0b00001 << 00);   // ⬛⬛⬛⬛⬜
        case MP_QSTR_ARROW_LEFT_DOWN:
            return (0b00001 << 20) |  // ⬛⬛⬛⬛⬜
                   (0b00010 << 15) |  // ⬛⬛⬛⬜⬛
                   (0b10100 << 10) |  // ⬜⬛⬜⬛⬛
                   (0b11000 << 05) |  // ⬜⬜⬛⬛⬛
                   (0b11100 << 00);   // ⬜⬜⬜⬛⬛
        case MP_QSTR_ARROW_UP:
            return (0b00100 << 20) |  // ⬛⬛⬜⬛⬛
                   (0b01110 << 15) |  // ⬛⬜⬜⬜⬛
                   (0b10101 << 10) |  // ⬜⬛⬜⬛⬜
                   (0b00100 << 05) |  // ⬛⬛⬜⬛⬛
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_ARROW_DOWN:
            return (0b00100 << 20) |  // ⬛⬛⬜⬛⬛
                   (0b00100 << 15) |  // ⬛⬛⬜⬛⬛
                   (0b10101 << 10) |  // ⬜⬛⬜⬛⬜
                   (0b01110 << 05) |  // ⬛⬜⬜⬜⬛
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_ARROW_LEFT:
            return (0b00100 << 20) |  // ⬛⬛⬜⬛⬛
                   (0b01000 << 15) |  // ⬛⬜⬛⬛⬛
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b01000 << 05) |  // ⬛⬜⬛⬛⬛
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_ARROW_RIGHT:
            return (0b00100 << 20) |  // ⬛⬛⬜⬛⬛
                   (0b00010 << 15) |  // ⬛⬛⬛⬜⬛
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b00010 << 05) |  // ⬛⬛⬛⬜⬛
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_HAPPY:
            return (0b11011 << 20) |  // ⬜⬜⬛⬜⬜
                   (0b11011 << 15) |  // ⬜⬜⬛⬜⬜
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b10001 << 05) |  // ⬜⬛⬛⬛⬜
                   (0b01110 << 00);   // ⬛⬜⬜⬜⬛
        case MP_QSTR_SAD:
            return (0b11011 << 20) |  // ⬜⬜⬛⬜⬜
                   (0b11011 << 15) |  // ⬜⬜⬛⬜⬜
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b01110 << 05) |  // ⬛⬜⬜⬜⬛
                   (0b10001 << 00);   // ⬜⬛⬛⬛⬜
        case MP_QSTR_EYE_LEFT:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 15) |  // ⬛⬛⬛⬛⬛
                   (0b11000 << 10) |  // ⬜⬜⬛⬛⬛
                   (0b11000 << 05) |  // ⬜⬜⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EYE_RIGHT:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 15) |  // ⬛⬛⬛⬛⬛
                   (0b00011 << 10) |  // ⬛⬛⬛⬜⬜
                   (0b00011 << 05) |  // ⬛⬛⬛⬜⬜
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EYE_LEFT_BLINK:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 15) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b11000 << 05) |  // ⬜⬜⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EYE_RIGHT_BLINK:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 15) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b00011 << 05) |  // ⬛⬛⬛⬜⬜
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EYE_RIGHT_BROW:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b00011 << 15) |  // ⬛⬛⬛⬜⬜
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 05) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EYE_LEFT_BROW:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b11000 << 15) |  // ⬜⬜⬛⬛⬛
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 05) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EYE_LEFT_BROW_UP:
            return (0b11000 << 20) |  // ⬜⬜⬛⬛⬛
                   (0b00000 << 15) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 05) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EYE_RIGHT_BROW_UP:
            return (0b00011 << 20) |  // ⬛⬛⬛⬜⬜
                   (0b00000 << 15) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 05) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_HEART:
            return (0b01010 << 20) |  // ⬛⬜⬛⬜⬛
                   (0b11111 << 15) |  // ⬜⬜⬜⬜⬜
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b01110 << 05) |  // ⬛⬜⬜⬜⬛
                   (0b00100 << 00);   // ⬛⬛⬜⬛⬛
        case MP_QSTR_PAUSE:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b01010 << 15) |  // ⬛⬜⬛⬜⬛
                   (0b01010 << 10) |  // ⬛⬜⬛⬜⬛
                   (0b01010 << 05) |  // ⬛⬜⬛⬜⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_EMPTY:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 15) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 10) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 05) |  // ⬛⬛⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_FULL:
            return (0b11111 << 20) |  // ⬜⬜⬜⬜⬜
                   (0b11111 << 15) |  // ⬜⬜⬜⬜⬜
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b11111 << 05) |  // ⬜⬜⬜⬜⬜
                   (0b11111 << 00);   // ⬜⬜⬜⬜⬜
        case MP_QSTR_SQUARE:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b01110 << 15) |  // ⬛⬜⬜⬜⬛
                   (0b01110 << 10) |  // ⬛⬜⬜⬜⬛
                   (0b01110 << 05) |  // ⬛⬜⬜⬜⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_TRIANGLE_RIGHT:
            return (0b01000 << 20) |  // ⬛⬜⬛⬛⬛
                   (0b01100 << 15) |  // ⬛⬜⬜⬛⬛
                   (0b01110 << 10) |  // ⬛⬜⬜⬜⬛
                   (0b01100 << 05) |  // ⬛⬜⬜⬛⬛
                   (0b01000 << 00);   // ⬛⬜⬛⬛⬛
        case MP_QSTR_TRIANGLE_LEFT:
            return (0b00010 << 20) |  // ⬛⬛⬛⬜⬛
                   (0b00110 << 15) |  // ⬛⬛⬜⬜⬛
                   (0b01110 << 10) |  // ⬛⬜⬜⬜⬛
                   (0b00110 << 05) |  // ⬛⬛⬜⬜⬛
                   (0b00010 << 00);   // ⬛⬛⬛⬜⬛
        case MP_QSTR_TRIANGLE_UP:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b00100 << 15) |  // ⬛⬛⬜⬛⬛
                   (0b01110 << 10) |  // ⬛⬜⬜⬜⬛
                   (0b11111 << 05) |  // ⬜⬜⬜⬜⬜
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_TRIANGLE_DOWN:
            return (0b00000 << 20) |  // ⬛⬛⬛⬛⬛
                   (0b11111 << 15) |  // ⬜⬜⬜⬜⬜
                   (0b01110 << 10) |  // ⬛⬜⬜⬜⬛
                   (0b00100 << 05) |  // ⬛⬛⬜⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_CIRCLE:
            return (0b01110 << 20) |  // ⬛⬜⬜⬜⬛
                   (0b11111 << 15) |  // ⬜⬜⬜⬜⬜
                   (0b11111 << 10) |  // ⬜⬜⬜⬜⬜
                   (0b11111 << 05) |  // ⬜⬜⬜⬜⬜
                   (0b01110 << 00);   // ⬛⬜⬜⬜⬛
        case MP_QSTR_CLOCKWISE:
            return (0b11110 << 20) |  // ⬜⬜⬜⬜⬛
                   (0b10010 << 15) |  // ⬜⬛⬛⬜⬛
                   (0b10010 << 10) |  // ⬜⬛⬛⬜⬛
                   (0b10111 << 05) |  // ⬜⬛⬜⬜⬜
                   (0b00010 << 00);   // ⬛⬛⬛⬜⬛
        case MP_QSTR_COUNTERCLOCKWISE:
            return (0b01111 << 20) |  // ⬛⬜⬜⬜⬜
                   (0b01001 << 15) |  // ⬛⬜⬛⬛⬜
                   (0b01001 << 10) |  // ⬛⬜⬛⬛⬜
                   (0b11101 << 05) |  // ⬜⬜⬜⬛⬜
                   (0b01000 << 00);   // ⬛⬜⬛⬛⬛
        case MP_QSTR_TRUE:
            return (0b00001 << 20) |  // ⬛⬛⬛⬛⬜
                   (0b00010 << 15) |  // ⬛⬛⬛⬜⬛
                   (0b10100 << 10) |  // ⬜⬛⬜⬛⬛
                   (0b01000 << 05) |  // ⬛⬜⬛⬛⬛
                   (0b00000 << 00);   // ⬛⬛⬛⬛⬛
        case MP_QSTR_FALSE:
            return (0b10001 << 20) |  // ⬜⬛⬛⬛⬜
                   (0b01010 << 15) |  // ⬛⬜⬛⬜⬛
                   (0b00100 << 10) |  // ⬛⬛⬜⬛⬛
                   (0b01010 << 05) |  // ⬛⬜⬛⬜⬛
                   (0b10001 << 00);   // ⬜⬛⬛⬛⬜
        default:
            return UINT32_MAX;
    }
}

// pybricks.parameters.Icon.ARROW_UP
// pybricks.parameters.Icon.ARROW_LEFT
// etc.
STATIC void pb_type_Icon_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    if (dest[0] == MP_OBJ_NULL) {
        uint32_t bitmap = get_bitmap(attr);
        if (bitmap != UINT32_MAX) {
            dest[0] = pb_type_Matrix_make_bitmap(5, 5, 100, bitmap);
        }
    }
}

// pybricks.parameters.Icon(x)
STATIC mp_obj_t pb_type_Icon_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    if (n_args != 1) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    return pb_type_Matrix_make_bitmap(5, 5, 100, mp_obj_get_int(args[0]));
}

STATIC const mp_obj_type_t pb_type_Icon = {
    { &mp_type_type },
    .name = MP_QSTR_Icon,
    .call = pb_type_Icon_call,
    .attr = pb_type_Icon_attr,
};

// We expose an instance instead of the type. This workaround allows
// us to provide class attributes via the attribute handler, generating
// the icons on the fly rather than storing them as 25 floats each.
const mp_obj_base_t pb_Icon_obj = {
    &pb_type_Icon
};

#endif // PYBRICKS_PY_PARAMETERS_ICON

#endif // PYBRICKS_PY_PARAMETERS
