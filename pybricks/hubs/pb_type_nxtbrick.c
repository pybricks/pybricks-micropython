// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_NXTBRICK

#include <pbsys/sys.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

typedef struct _hubs_NXTBrick_obj_t {
    mp_obj_base_t base;
} hubs_NXTBrick_obj_t;

STATIC mp_obj_t hubs_NXTBrick_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_NXTBrick_obj_t *self = m_new_obj(hubs_NXTBrick_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t hubs_NXTBrick_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
};
STATIC MP_DEFINE_CONST_DICT(hubs_NXTBrick_locals_dict, hubs_NXTBrick_locals_dict_table);

const mp_obj_type_t pb_type_NXTBrick = {
    { &mp_type_type },
    .name = MP_QSTR_NXTBrick,
    .make_new = hubs_NXTBrick_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_NXTBrick_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_NXTBRICK
