// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_EV3BRICK
#include <pbio/util.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include "pb_ev3dev_types.h"


// defined in pbio/platform/ev3dev_stretch/status_light.c
extern pbio_color_light_t *ev3dev_status_light;

typedef struct _hubs_EV3Brick_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    mp_obj_t screen;
    mp_obj_t speaker;
    mp_obj_t buttons;
} hubs_EV3Brick_obj_t;

static const pb_obj_enum_member_t *ev3brick_buttons[] = {
    &pb_Button_LEFT_obj,
    &pb_Button_RIGHT_obj,
    &pb_Button_UP_obj,
    &pb_Button_DOWN_obj,
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_EV3Brick_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_EV3Brick_obj_t *self = m_new_obj(hubs_EV3Brick_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    mp_obj_t screen_args[] = { MP_ROM_QSTR(MP_QSTR__screen_) };
    self->screen = pb_type_ev3dev_Image.make_new(&pb_type_ev3dev_Image, 1, 0, screen_args);

    self->speaker = pb_type_ev3dev_Speaker.make_new(&pb_type_ev3dev_Speaker, 0, 0, NULL);

    self->light = common_ColorLight_internal_obj_new(ev3dev_status_light);

    self->buttons = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(ev3brick_buttons), ev3brick_buttons, pbio_button_is_pressed);

    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t attribute_table[] = {
    PB_DEFINE_CONST_ATTR_RO(hubs_EV3Brick_obj_t, MP_QSTR_buttons, buttons),
    PB_DEFINE_CONST_ATTR_RO(hubs_EV3Brick_obj_t, MP_QSTR_light, light),
    PB_DEFINE_CONST_ATTR_RO(hubs_EV3Brick_obj_t, MP_QSTR_screen, screen),
    PB_DEFINE_CONST_ATTR_RO(hubs_EV3Brick_obj_t, MP_QSTR_speaker, speaker),
};
STATIC MP_DEFINE_CONST_DICT(attribute_dict, attribute_table);

STATIC const mp_rom_map_elem_t hubs_EV3Brick_locals_dict_table[] = {
    PB_ATTRIBUTE_TABLE(attribute_dict),
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)      },
    { MP_ROM_QSTR(MP_QSTR_system),      MP_ROM_PTR(&pb_type_System)                           },
};
STATIC MP_DEFINE_CONST_DICT(hubs_EV3Brick_locals_dict, hubs_EV3Brick_locals_dict_table);

const mp_obj_type_t pb_type_ThisHub = {
    { &mp_type_type },
    .name = PYBRICKS_HUB_CLASS_NAME,
    .make_new = hubs_EV3Brick_make_new,
    .attr = pb_attribute_handler,
    .locals_dict = (mp_obj_dict_t *)&hubs_EV3Brick_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_EV3BRICK
