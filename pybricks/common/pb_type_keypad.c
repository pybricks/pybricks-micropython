// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_KEYPAD

#include <pbio/button.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_pb/pb_error.h>

// pybricks._common.Keypad class object
typedef struct _common_Keypad_obj_t {
    mp_obj_base_t base;
    uint8_t number_of_buttons;
    const pb_obj_enum_member_t **buttons;
} common_Keypad_obj_t;

// pybricks._common.Keypad.pressed
STATIC mp_obj_t common_Keypad_pressed(mp_obj_t self_in) {
    common_Keypad_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Read button combination code.
    pbio_button_flags_t pressed;
    pb_assert(pbio_button_is_pressed(&pressed));

    mp_obj_t button_list[PBIO_BUTTON_NUM_BUTTONS];
    uint8_t size = 0;

    for (uint8_t i = 0; i < self->number_of_buttons; i++) {
        if (pressed & self->buttons[i]->value) {
            button_list[size++] = MP_OBJ_FROM_PTR(self->buttons[i]);
        }
    }

    return mp_obj_new_tuple(size, button_list);
}
MP_DEFINE_CONST_FUN_OBJ_1(common_Keypad_pressed_obj, common_Keypad_pressed);

// dir(pybricks.common.Keypad)
STATIC const mp_rom_map_elem_t common_Keypad_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_pressed),     MP_ROM_PTR(&common_Keypad_pressed_obj)     },
};
STATIC MP_DEFINE_CONST_DICT(common_Keypad_locals_dict, common_Keypad_locals_dict_table);

// type(pybricks.common.Keypad)
STATIC const mp_obj_type_t pb_type_Keypad = {
    { &mp_type_type },
    .name = MP_QSTR_Keypad,
    .locals_dict = (mp_obj_dict_t *)&common_Keypad_locals_dict,
};

// pybricks._common.Keypad.__init__
mp_obj_t pb_type_Keypad_obj_new(uint8_t number_of_buttons, const pb_obj_enum_member_t **buttons) {
    // Create new light instance
    common_Keypad_obj_t *self = m_new_obj(common_Keypad_obj_t);
    self->base.type = &pb_type_Keypad;

    // Store hub specific button info
    self->number_of_buttons = number_of_buttons;
    self->buttons = buttons;

    return self;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_KEYPAD
