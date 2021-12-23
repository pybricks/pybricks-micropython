// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_CITYHUB

#include <pbdrv/reset.h>
#include <pbsys/light.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include <pybricks/util_mp/pb_obj_helper.h>

typedef struct _hubs_CityHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t battery;
    mp_obj_t button;
    mp_obj_t light;
    mp_obj_t system;
} hubs_CityHub_obj_t;

static const pb_obj_enum_member_t *cityhub_buttons[] = {
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_CityHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_CityHub_obj_t *self = m_new_obj(hubs_CityHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    self->button = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(cityhub_buttons), cityhub_buttons, pbio_button_is_pressed);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const pb_attr_dict_entry_t hubs_CityHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_CityHub_obj_t, battery),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_button, hubs_CityHub_obj_t, button),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_CityHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_CityHub_obj_t, system),
};

const pb_obj_with_attr_type_t pb_type_ThisHub = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = MP_QSTR_CityHub,
        .make_new = hubs_CityHub_make_new,
        .attr = pb_attribute_handler,
    },
    .attr_dict = hubs_CityHub_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(hubs_CityHub_attr_dict),
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_CITYHUB
