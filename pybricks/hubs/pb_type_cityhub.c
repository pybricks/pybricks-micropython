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
    mp_obj_t button;
    mp_obj_t light;
} hubs_CityHub_obj_t;

static const pb_obj_enum_member_t *cityhub_buttons[] = {
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_CityHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_CityHub_obj_t *self = m_new_obj(hubs_CityHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->button = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(cityhub_buttons), cityhub_buttons, pbio_button_is_pressed);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t attribute_table[] = {
    PB_DEFINE_CONST_ATTR_RO(hubs_CityHub_obj_t, MP_QSTR_button, button),
    PB_DEFINE_CONST_ATTR_RO(hubs_CityHub_obj_t, MP_QSTR_light, light),
};
STATIC MP_DEFINE_CONST_DICT(hubs_CityHub_attr_dict, attribute_table);

STATIC const mp_rom_map_elem_t hubs_CityHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_system),      MP_ROM_PTR(&pb_type_System) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_CityHub_locals_dict, hubs_CityHub_locals_dict_table);

const pb_obj_with_attr_type_t pb_type_ThisHub = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = MP_QSTR_CityHub,
        .make_new = hubs_CityHub_make_new,
        .attr = pb_attribute_handler,
        .locals_dict = (mp_obj_dict_t *)&hubs_CityHub_locals_dict,
    },
    .attr_dict = (mp_obj_dict_t *)&hubs_CityHub_attr_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_CITYHUB
