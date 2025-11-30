// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_BUILDHAT

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

typedef struct _hubs_BuildHat_obj_t {
    mp_obj_base_t base;
    mp_obj_t system;
} hubs_BuildHat_obj_t;

static mp_obj_t hubs_BuildHat_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    hubs_BuildHat_obj_t *self = mp_obj_malloc(hubs_BuildHat_obj_t, type);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

static const pb_attr_dict_entry_t hubs_BuildHat_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_BuildHat_obj_t, system),
    PB_ATTR_DICT_SENTINEL
};

MP_DEFINE_CONST_OBJ_TYPE(pb_type_ThisHub,
    MP_QSTR_BuildHat,
    MP_TYPE_FLAG_NONE,
    make_new, hubs_BuildHat_make_new,
    attr, pb_attribute_handler,
    protocol, hubs_BuildHat_attr_dict);

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_BUILDHAT
