// SPDX-License-Identifier: MIT
// Copyright (c) 2022 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_VIRTUALHUB

#include "py/runtime.h"
#include "py/obj.h"

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

typedef struct _hubs_VirtualHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t system;
} hubs_VirtualHub_obj_t;

STATIC mp_obj_t hubs_VirtualHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_VirtualHub_obj_t *self = m_new_obj(hubs_VirtualHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const pb_attr_dict_entry_t hubs_VirtualHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_VirtualHub_obj_t, system),
};

const pb_obj_with_attr_type_t pb_type_ThisHub = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = PYBRICKS_HUB_CLASS_NAME,
        .make_new = hubs_VirtualHub_make_new,
        .attr = pb_attribute_handler,
    },
    .attr_dict = hubs_VirtualHub_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(hubs_VirtualHub_attr_dict),
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_VIRTUALHUB
