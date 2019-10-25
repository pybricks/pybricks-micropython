// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/port.h>
#include <pbio/button.h>

#include "modparameters.h"

#include "pberror.h"
#include "pbobj.h"

// Class structure for EV3Brick
typedef struct _hubs_EV3Brick_obj_t {
    mp_obj_base_t base;
} hubs_EV3Brick_obj_t;

STATIC void hubs_EV3Brick_print(const mp_print_t *print,  mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, qstr_str(MP_QSTR_EV3Brick));
}

STATIC mp_obj_t hubs_EV3Brick_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args ) {
    // Initialize self
    hubs_EV3Brick_obj_t *self = m_new_obj(hubs_EV3Brick_obj_t);
    self->base.type = (mp_obj_type_t*) type;
    return MP_OBJ_FROM_PTR(self);
}

extern const struct _mp_obj_module_t pb_module_buttons;
extern const struct _mp_obj_module_t pb_module_battery;
extern const struct _mp_obj_module_t pb_module_colorlight;

/*
EV3Brick class tables
*/
STATIC const mp_rom_map_elem_t hubs_EV3Brick_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_PTR(&pb_module_buttons)      },
    { MP_ROM_QSTR(MP_QSTR_Port),        MP_ROM_PTR(&pb_enum_type_Port)      },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_PTR(&pb_module_colorlight)   },
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)      },
};
STATIC MP_DEFINE_CONST_DICT(hubs_EV3Brick_locals_dict, hubs_EV3Brick_locals_dict_table);

STATIC const mp_obj_type_t hubs_EV3Brick_type = {
    { &mp_type_type },
    .name = MP_QSTR_EV3Brick,
    .print = hubs_EV3Brick_print,
    .make_new = hubs_EV3Brick_make_new,
    .locals_dict = (mp_obj_dict_t*)&hubs_EV3Brick_locals_dict,
};

/* Module table */

STATIC const mp_rom_map_elem_t hubs_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),    MP_ROM_QSTR(MP_QSTR_hubs )      },
    { MP_ROM_QSTR(MP_QSTR_EV3Brick),    MP_ROM_PTR(&hubs_EV3Brick_type) },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_hubs_globals, hubs_globals_table);

const mp_obj_module_t pb_module_hubs = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pb_module_hubs_globals,
};
