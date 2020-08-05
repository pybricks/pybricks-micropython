// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_MOVEHUB

#include <pbsys/sys.h>

#include "py/mphal.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/hubs.h>


typedef struct _hubs_MoveHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t light;
} hubs_MoveHub_obj_t;

STATIC mp_obj_t hubs_MoveHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_MoveHub_obj_t *self = m_new_obj(hubs_MoveHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->light = common_ColorLight_obj_make_new(NULL);
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t hubs_MoveHub_shutdown(mp_obj_t self_in) {
    pbsys_power_off();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hubs_movehub_shutdown_obj, hubs_MoveHub_shutdown);

STATIC mp_obj_t hubs_MoveHub_reboot(mp_obj_t self_in) {
    pbsys_reboot(0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hubs_movehub_reboot_obj, hubs_MoveHub_reboot);

STATIC mp_obj_t hubs_MoveHub_update(mp_obj_t self_in) {
    pbsys_reboot(1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(hubs_movehub_update_obj, hubs_MoveHub_update);

STATIC const mp_rom_map_elem_t hubs_MoveHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_shutdown),    MP_ROM_PTR(&hubs_movehub_shutdown_obj)     },
    { MP_ROM_QSTR(MP_QSTR_reboot),      MP_ROM_PTR(&hubs_movehub_reboot_obj)       },
    { MP_ROM_QSTR(MP_QSTR_update),      MP_ROM_PTR(&hubs_movehub_update_obj)       },
    { MP_ROM_QSTR(MP_QSTR_light),       MP_ROM_ATTRIBUTE_OFFSET(hubs_MoveHub_obj_t, light) },
};
STATIC MP_DEFINE_CONST_DICT(hubs_MoveHub_locals_dict, hubs_MoveHub_locals_dict_table);

const mp_obj_type_t pb_type_MoveHub = {
    { &mp_type_type },
    .name = MP_QSTR_MoveHub,
    .make_new = hubs_MoveHub_make_new,
    .locals_dict = (mp_obj_dict_t *)&hubs_MoveHub_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_MOVEHUB
