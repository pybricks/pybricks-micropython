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
        case MP_QSTR_ARROW_UP:
            return (0b00100 << 20) | // ⬛⬛⬜⬛⬛
                   (0b01110 << 15) | // ⬛⬜⬜⬜⬛
                   (0b11111 << 10) | // ⬜⬜⬜⬜⬜
                   (0b01110 << 05) | // ⬛⬜⬜⬜⬛
                   (0b01110 << 00);  // ⬛⬜⬜⬜⬛
        case MP_QSTR_ARROW_LEFT:
            return (0b00100 << 20) | // ⬛⬛⬜⬛⬛
                   (0b01111 << 15) | // ⬛⬜⬜⬜⬜
                   (0b11111 << 10) | // ⬜⬜⬜⬜⬜
                   (0b01111 << 05) | // ⬛⬜⬜⬜⬜
                   (0b00110 << 00);  // ⬛⬛⬜⬛⬛
        case MP_QSTR_ARROW_RIGHT:
            return (0b00100 << 20) | // ⬛⬛⬜⬛⬛
                   (0b11110 << 15) | // ⬜⬜⬜⬜⬛
                   (0b11111 << 10) | // ⬜⬜⬜⬜⬜
                   (0b11110 << 05) | // ⬜⬜⬜⬜⬛
                   (0b00100 << 00);  // ⬛⬛⬜⬛⬛
        case MP_QSTR_ARROW_DOWN:
            return (0b01110 << 20) | // ⬛⬜⬜⬜⬛
                   (0b01110 << 15) | // ⬛⬜⬜⬜⬛
                   (0b11111 << 10) | // ⬜⬜⬜⬜⬜
                   (0b01110 << 05) | // ⬛⬜⬜⬜⬛
                   (0b00100 << 00);  // ⬛⬛⬜⬛⬛
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
