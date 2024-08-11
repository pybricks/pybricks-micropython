// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS

#if PYBRICKS_PY_PARAMETERS_BUTTON

#include <pbio/button.h>

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/parameters/pb_type_button.h>

#include <pybricks/util_mp/pb_type_enum.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include <pybricks/util_pb/pb_error.h>

static const qstr buttons[] = {
    MP_QSTR_LEFT,
    MP_QSTR_RIGHT,
    MP_QSTR_CENTER,
    MP_QSTR_LEFT_PLUS,
    MP_QSTR_LEFT_MINUS,
    MP_QSTR_RIGHT_PLUS,
    MP_QSTR_RIGHT_MINUS,
    #if !PYBRICKS_PY_PARAMETERS_BUTTON_REMOTE_ONLY
    MP_QSTR_UP,
    MP_QSTR_DOWN,
    MP_QSTR_LEFT_UP,
    MP_QSTR_LEFT_DOWN,
    MP_QSTR_RIGHT_UP,
    MP_QSTR_RIGHT_DOWN,
    MP_QSTR_BEACON,
    MP_QSTR_BLUETOOTH,
    MP_QSTR_A,
    MP_QSTR_B,
    MP_QSTR_X,
    MP_QSTR_Y,
    MP_QSTR_LB,
    MP_QSTR_RB,
    MP_QSTR_VIEW,
    MP_QSTR_MENU,
    MP_QSTR_GUIDE,
    MP_QSTR_LJ,
    MP_QSTR_RJ,
    MP_QSTR_UPLOAD,
    MP_QSTR_P1,
    MP_QSTR_P2,
    MP_QSTR_P3,
    MP_QSTR_P4,
    #endif
};

extern const mp_obj_type_t pb_type_button_;

static void pb_type_button_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    pb_obj_button_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, self->name == pb_type_button.name ? "%q" : "%q.%q", MP_QSTR_Button, self->name);
}

mp_obj_t pb_type_button_new(qstr name) {
    pb_obj_button_t *result = mp_obj_malloc(pb_obj_button_t, &pb_type_button_);
    result->name = name;
    return MP_OBJ_FROM_PTR(result);
}

static void pb_type_button_attribute_handler(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {

    // Write and delete not supported. Re-reading from an instance also not supported.
    if (dest[0] != MP_OBJ_NULL || MP_OBJ_TO_PTR(self_in) != &pb_type_button) {
        return;
    }

    // Allocate new button object if the attribute is a valid button.
    for (size_t i = 0; i < MP_ARRAY_SIZE(buttons); i++) {
        if (attr == buttons[i]) {
            dest[0] = pb_type_button_new(attr);
            return;
        }
    }
}

static mp_obj_t pb_type_button_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {

    // Only equality comparison is supported.
    if (op != MP_BINARY_OP_EQUAL) {
        return MP_OBJ_NULL;
    }
    if (!mp_obj_is_type(lhs_in, &pb_type_button_) || !mp_obj_is_type(rhs_in, &pb_type_button_)) {
        return mp_const_false;
    }

    pb_obj_button_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    pb_obj_button_t *rhs = MP_OBJ_TO_PTR(rhs_in);
    return mp_obj_new_bool(lhs->name == rhs->name);
}

MP_DEFINE_CONST_OBJ_TYPE(pb_type_button_,
    MP_QSTR_Button,
    MP_TYPE_FLAG_NONE,
    print, pb_type_button_print,
    attr, pb_type_button_attribute_handler,
    unary_op, mp_generic_unary_op,
    binary_op, pb_type_button_binary_op
    );

// The exposed 'type' object is in fact an instance of the type, so we can have
// an attribute handler for the type that creates new button objects.
const pb_obj_button_t pb_type_button = {
    {&pb_type_button_},
    .name = MP_QSTRnull
};

#if PYBRICKS_PY_COMMON_KEYPAD_HUB_BUTTONS > 1
pbio_button_flags_t pb_type_button_get_button_flag(mp_obj_t obj) {
    pb_assert_type(obj, &pb_type_button_);
    pb_obj_button_t *button = MP_OBJ_TO_PTR(obj);

    switch (button->name) {
        case MP_QSTR_UP:
        case MP_QSTR_BEACON:
            return PBIO_BUTTON_UP;
        case MP_QSTR_DOWN:
            return PBIO_BUTTON_DOWN;
        case MP_QSTR_LEFT:
            return PBIO_BUTTON_LEFT;
        case MP_QSTR_RIGHT:
            return PBIO_BUTTON_RIGHT;
        case MP_QSTR_CENTER:
            return PBIO_BUTTON_CENTER;
        case MP_QSTR_LEFT_UP:
        case MP_QSTR_LEFT_PLUS:
            return PBIO_BUTTON_LEFT_UP;
        case MP_QSTR_LEFT_DOWN:
        case MP_QSTR_LEFT_MINUS:
            return PBIO_BUTTON_LEFT_DOWN;
        case MP_QSTR_RIGHT_UP:
        case MP_QSTR_RIGHT_PLUS:
        case MP_QSTR_BLUETOOTH:
            return PBIO_BUTTON_RIGHT_UP;
        case MP_QSTR_RIGHT_DOWN:
        case MP_QSTR_RIGHT_MINUS:
            return PBIO_BUTTON_RIGHT_DOWN;
        default:
            return 0;
    }
}
#endif

/**
 * Common button pressed function for single button hubs.
 */
mp_obj_t pb_type_button_pressed_hub_single_button(void) {
    pbio_button_flags_t flags;
    pb_assert(pbio_button_is_pressed(&flags));
    mp_obj_t buttons[] = { pb_type_button_new(MP_QSTR_CENTER) };

    #if MICROPY_PY_BUILTINS_SET
    return mp_obj_new_set(flags ? MP_ARRAY_SIZE(buttons) : 0, buttons);
    #else
    return mp_obj_new_tuple(flags ? MP_ARRAY_SIZE(buttons) : 0, buttons);
    #endif
}

#endif // PYBRICKS_PY_PARAMETERS_BUTTON

#endif // PYBRICKS_PY_PARAMETERS
