// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_CITYHUB

#include <pbdrv/reset.h>
#include <pbsys/light.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

typedef struct _hubs_CityHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
} hubs_CityHub_obj_t;

STATIC mp_obj_t hubs_CityHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_CityHub_obj_t *self = m_new_obj(hubs_CityHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t hubs_CityHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_CityHub_obj_t, light) },
    { MP_ROM_QSTR(MP_QSTR_reset),       MP_ROM_PTR(&pb_hubs_Hub_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_reason),MP_ROM_PTR(&pb_hubs_Hub_reset_reason_obj) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_CityHub_locals_dict, hubs_CityHub_locals_dict_table);

const mp_obj_type_t pb_type_ThisHub = {
    { &mp_type_type },
    .name = MP_QSTR_CityHub,
    .make_new = hubs_CityHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_CityHub_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_CITYHUB
