// SPDX-License-Identifier: MIT
// Copyright (c) 2018 Laurens Valk

#include <pbio/port.h>
#include <pbio/button.h>

#include "pberror.h"
#include "pbobj.h"

STATIC mp_obj_t ev3brick_buttons(void) {
    mp_obj_t button_list[6];
    pbio_button_flags_t pressed;
    uint8_t size = 0;

    pb_assert(pbio_button_is_pressed(&pressed));

    if (pressed & PBIO_BUTTON_CENTER) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_CENTER);
    }
    if (pressed & PBIO_BUTTON_LEFT) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_LEFT);
    }
    if (pressed & PBIO_BUTTON_RIGHT) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_RIGHT);
    }
    if (pressed & PBIO_BUTTON_UP) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_UP);
    }
    if (pressed & PBIO_BUTTON_DOWN) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_DOWN);
    }
    if (pressed & PBIO_BUTTON_LEFT_UP) {
        button_list[size++] = mp_obj_new_int(PBIO_BUTTON_LEFT_UP);
    }

    return mp_obj_new_list(size, button_list);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(ev3brick_buttons_obj, ev3brick_buttons);

/* Module table */

extern const struct _mp_obj_module_t pb_module_battery;
extern const struct _mp_obj_module_t pb_module_colorlight;

STATIC const mp_rom_map_elem_t ev3brick_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_ev3brick )      },
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_PTR(&ev3brick_buttons_obj)   },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_PTR(&pb_module_colorlight)   }, 
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)      },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_ev3brick_globals, ev3brick_globals_table);

const mp_obj_module_t pb_module_ev3brick = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_ev3brick_globals,
};
