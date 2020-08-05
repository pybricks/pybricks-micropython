// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_EV3BRICK

#include <pbsys/sys.h>

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include "pb_ev3dev_types.h"

typedef struct _hubs_EV3Brick_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    mp_obj_t screen;
    mp_obj_t speaker;
} hubs_EV3Brick_obj_t;

STATIC mp_obj_t hubs_EV3Brick_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_EV3Brick_obj_t *self = m_new_obj(hubs_EV3Brick_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_obj_t screen_args[] = { MP_ROM_QSTR(MP_QSTR__screen_) };
    self->screen = pb_type_ev3dev_Image.make_new(&pb_type_ev3dev_Image, 1, 0, screen_args);

    self->speaker = pb_type_ev3dev_Speaker.make_new(&pb_type_ev3dev_Speaker, 0, 0, NULL);

    self->light = common_ColorLight_obj_make_new(NULL);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t hubs_EV3Brick_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)      },
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_PTR(&pb_module_buttons)      },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_EV3Brick_obj_t, light)   },
    { MP_ROM_QSTR(MP_QSTR_screen),      MP_ROM_ATTRIBUTE_OFFSET(hubs_EV3Brick_obj_t, screen)  },
    { MP_ROM_QSTR(MP_QSTR_speaker),     MP_ROM_ATTRIBUTE_OFFSET(hubs_EV3Brick_obj_t, speaker) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_EV3Brick_locals_dict, hubs_EV3Brick_locals_dict_table);

const mp_obj_type_t pb_type_EV3Brick = {
    { &mp_type_type },
    .name = MP_QSTR_EV3Brick,
    .make_new = hubs_EV3Brick_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_EV3Brick_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_EV3BRICK
