// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_BUTTONS

#include "py/obj.h"

#include <pbio/button.h>

#include "pberror.h"
#include "modparameters.h"

STATIC mp_obj_t buttons_pressed(void) {
    mp_obj_t button_list[10];
    pbio_button_flags_t pressed;
    uint8_t size = 0;

    pb_assert(pbio_button_is_pressed(&pressed));

    if (pressed & PBIO_BUTTON_CENTER) {
        button_list[size++] = pb_const_button_center;
    }
    if (pressed & PBIO_BUTTON_LEFT) {
        button_list[size++] = pb_const_button_left;
    }
    if (pressed & PBIO_BUTTON_RIGHT) {
        button_list[size++] = pb_const_button_right;
    }
    if (pressed & PBIO_BUTTON_UP) {
        button_list[size++] = pb_const_button_up;
    }
    if (pressed & PBIO_BUTTON_DOWN) {
        button_list[size++] = pb_const_button_down;
    }
    if (pressed & PBIO_BUTTON_LEFT_UP) {
        button_list[size++] = pb_const_button_left_up;
    }

    return mp_obj_new_list(size, button_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(buttons_pressed_obj, buttons_pressed);

/* buttons module tables */

STATIC const mp_rom_map_elem_t buttons_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_buttons)        },
    { MP_ROM_QSTR(MP_QSTR_pressed),     MP_ROM_PTR(&buttons_pressed_obj)    },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_buttons_globals, buttons_globals_table);

const mp_obj_module_t pb_module_buttons = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_buttons_globals,
};

#endif // PYBRICKS_PY_BUTTONS
