// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_KEYPAD

#include <pbio/button.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/parameters/pb_type_button.h>

#include <pybricks/util_pb/pb_error.h>

// pybricks._common.Keypad class object
typedef struct _common_Keypad_obj_t {
    mp_obj_base_t base;
    pb_type_button_get_pressed_t get_pressed;
} common_Keypad_obj_t;

// pybricks._common.Keypad.pressed
static mp_obj_t common_Keypad_pressed(mp_obj_t self_in) {
    common_Keypad_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->get_pressed();
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Keypad_pressed_obj, common_Keypad_pressed);

// dir(pybricks.common.Keypad)
static const mp_rom_map_elem_t common_Keypad_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pressed),     MP_ROM_PTR(&common_Keypad_pressed_obj)     },
};
static MP_DEFINE_CONST_DICT(common_Keypad_locals_dict, common_Keypad_locals_dict_table);

// type(pybricks.common.Keypad)
static MP_DEFINE_CONST_OBJ_TYPE(pb_type_Keypad,
    MP_QSTRnull,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_Keypad_locals_dict);

// pybricks._common.Keypad.__init__
mp_obj_t pb_type_Keypad_obj_new(pb_type_button_get_pressed_t get_pressed) {
    common_Keypad_obj_t *self = mp_obj_malloc(common_Keypad_obj_t, &pb_type_Keypad);
    self->get_pressed = get_pressed;
    return MP_OBJ_FROM_PTR(self);
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_KEYPAD
