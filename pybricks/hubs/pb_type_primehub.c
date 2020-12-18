// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_PRIMEHUB

#if !PYBRICKS_PY_COMMON_LIGHT_MATRIX || !PYBRICKS_PY_PARAMETERS_BUTTON
#error "PYBRICKS_PY_COMMON_LIGHT_MATRIX and PYBRICKS_PY_PARAMETERS_BUTTON must be enabled."
#endif

#include <pbsys/light.h>

#include "py/runtime.h"
#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/hubs.h>

typedef struct _hubs_PrimeHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    mp_obj_t display;
    mp_obj_t speaker;
} hubs_PrimeHub_obj_t;

STATIC mp_obj_t hubs_PrimeHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_PrimeHub_obj_t *self = m_new_obj(hubs_PrimeHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->display = pb_type_Lightmatrix_obj_new(pbsys_hub_light_matrix);
    self->speaker = mp_call_function_0(MP_OBJ_FROM_PTR(&pb_type_Speaker));
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t hubs_PrimeHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_buttons),     MP_ROM_PTR(&pb_module_buttons)    },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_PrimeHub_obj_t, light)   },
    { MP_ROM_QSTR(MP_QSTR_display),     MP_ROM_ATTRIBUTE_OFFSET(hubs_PrimeHub_obj_t, display) },
    { MP_ROM_QSTR(MP_QSTR_speaker),     MP_ROM_ATTRIBUTE_OFFSET(hubs_PrimeHub_obj_t, speaker) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_PrimeHub_locals_dict, hubs_PrimeHub_locals_dict_table);

const mp_obj_type_t pb_type_PrimeHub = {
    { &mp_type_type },
    .name = MP_QSTR_PrimeHub,
    .make_new = hubs_PrimeHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_PrimeHub_locals_dict,
};

const mp_obj_type_t pb_type_InventorHub = {
    { &mp_type_type },
    .name = MP_QSTR_InventorHub,
    .make_new = hubs_PrimeHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_PrimeHub_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_PRIMEHUB
