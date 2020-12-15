// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

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
} common_Keypad_obj_t;

// pybricks._common.Keypad.pressed
STATIC mp_obj_t common_Keypad_pressed(mp_obj_t self_in) {
    // common_Keypad_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t button_list[10];
    pbio_button_flags_t pressed;
    uint8_t size = 0;

    pb_assert(pbio_button_is_pressed(&pressed));

    if (pressed & PBIO_BUTTON_CENTER) {
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_CENTER_obj);
    }
    if (pressed & PBIO_BUTTON_LEFT) {
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_LEFT_obj);
    }
    if (pressed & PBIO_BUTTON_RIGHT) {
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_RIGHT_obj);
    }
    // FIXME: pass available buttons when initializing button object
    if (pressed & PBIO_BUTTON_RIGHT_UP) {
        #ifdef PYBRICKS_HUB_PRIMEHUB
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_BT_obj);
        #else
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_RIGHT_UP_obj);
        #endif
    }
    if (pressed & PBIO_BUTTON_UP) {
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_UP_obj);
    }
    if (pressed & PBIO_BUTTON_DOWN) {
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_DOWN_obj);
    }
    if (pressed & PBIO_BUTTON_LEFT_UP) {
        button_list[size++] = MP_OBJ_FROM_PTR(&pb_Button_LEFT_UP_obj);
    }

    return mp_obj_new_list(size, button_list);
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
mp_obj_t pb_type_Keypad_obj_new() {
    // Create new light instance
    common_Keypad_obj_t *self = m_new_obj(common_Keypad_obj_t);
    self->base.type = &pb_type_Keypad;
    return self;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_KEYPAD
