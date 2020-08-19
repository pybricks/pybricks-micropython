// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_PRIMEHUB

#if !PYBRICKS_PY_COMMON_LIGHTGRID
#error "PYBRICKS_PY_COMMON_LIGHTGRID must be enabled if PYBRICKS_HUB_PRIMEHUB is enabled."
#endif

#include <pbsys/sys.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

static uint8_t spike_prime_tlc5955_pwm_channel_map[] = {
    38, 36, 41, 46, 33,
    37, 28, 39, 47, 21,
    24, 29, 31, 45, 23,
    26, 27, 32, 34, 22,
    25, 40, 30, 35, 9,
};

typedef struct _hubs_PrimeHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
    mp_obj_t grid;
} hubs_PrimeHub_obj_t;

STATIC mp_obj_t hubs_PrimeHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_PrimeHub_obj_t *self = m_new_obj(hubs_PrimeHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->light = common_ColorLight_internal_obj_make_new();
    self->grid = common_LightGrid_obj_make_new(4, spike_prime_tlc5955_pwm_channel_map, 5);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t hubs_PrimeHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_PrimeHub_obj_t, light) },
    { MP_ROM_QSTR(MP_QSTR_grid),        MP_ROM_ATTRIBUTE_OFFSET(hubs_PrimeHub_obj_t, grid)  },
};
STATIC MP_DEFINE_CONST_DICT(hubs_PrimeHub_locals_dict, hubs_PrimeHub_locals_dict_table);

const mp_obj_type_t pb_type_PrimeHub = {
    { &mp_type_type },
    .name = MP_QSTR_PrimeHub,
    .make_new = hubs_PrimeHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_PrimeHub_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_PRIMEHUB
