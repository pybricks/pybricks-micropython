// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

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
    mp_obj_t battery;
    mp_obj_t buttons;
    mp_obj_t light;
    mp_obj_t screen;
    mp_obj_t speaker;
    mp_obj_t system;
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

    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    self->buttons = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(ev3brick_buttons), ev3brick_buttons, pbio_button_is_pressed);
    self->light = common_ColorLight_internal_obj_new(ev3dev_status_light);
    mp_obj_t screen_args[] = { MP_ROM_QSTR(MP_QSTR__screen_) };
    self->screen = pb_type_ev3dev_Image.type.make_new(&pb_type_ev3dev_Image.type, 1, 0, screen_args);
    self->speaker = pb_type_ev3dev_Speaker.make_new(&pb_type_ev3dev_Speaker, 0, 0, NULL);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);

    return MP_OBJ_FROM_PTR(self);
}

STATIC const pb_attr_dict_entry_t hubs_EV3Brick_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_EV3Brick_obj_t, battery),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, hubs_EV3Brick_obj_t, buttons),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_EV3Brick_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_screen, hubs_EV3Brick_obj_t, screen),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_speaker, hubs_EV3Brick_obj_t, speaker),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_EV3Brick_obj_t, system),
};

const pb_obj_with_attr_type_t pb_type_ThisHub = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = PYBRICKS_HUB_CLASS_NAME,
        .make_new = hubs_EV3Brick_make_new,
        .attr = pb_attribute_handler,
    },
    .attr_dict = hubs_EV3Brick_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(hubs_EV3Brick_attr_dict),
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_EV3BRICK
