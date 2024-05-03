// SPDX-License-Identifier: MIT
// Copyright (c) 2024 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_TOOLS_HOSTBUFFER

#include "py/mphal.h"

#include <pybricks/tools.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct _pb_type_hostbuffer_obj_t {
    mp_obj_base_t base;
} pb_type_hostbuffer_obj_t;

STATIC mp_obj_t pb_type_hostbuffer_time(mp_obj_t self_in) {
    pb_type_hostbuffer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    (void)self;
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_hostbuffer_time_obj, pb_type_hostbuffer_time);

STATIC mp_obj_t pb_type_hostbuffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    pb_type_hostbuffer_obj_t *self = mp_obj_malloc(pb_type_hostbuffer_obj_t, type);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t pb_type_hostbuffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&pb_type_hostbuffer_time_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_hostbuffer_locals_dict, pb_type_hostbuffer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_hostbuffer,
    MP_QSTR_hostbuffer,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_hostbuffer_make_new,
    locals_dict, &pb_type_hostbuffer_locals_dict);

#endif // PYBRICKS_PY_TOOLS_HOSTBUFFER
