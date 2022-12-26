// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_BLE

#include <pybricks/ble.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_conversions.h>

// Class structure for Broadcast
typedef struct _ble_Broadcast_obj_t {
    mp_obj_base_t base;
} ble_Broadcast_obj_t;

// pybricks.ble.Broadcast.__init__
STATIC mp_obj_t ble_Broadcast_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(arg));

    ble_Broadcast_obj_t *self = m_new_obj(ble_Broadcast_obj_t);
    self->base.type = (mp_obj_type_t *)type;

    (void)arg_in;

    return MP_OBJ_FROM_PTR(self);
}

// pybricks.ble.Broadcast.method
STATIC mp_obj_t ble_Broadcast_method(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ble_Broadcast_obj_t, self,
        PB_ARG_REQUIRED(arg));

    (void)self;
    (void)arg_in;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ble_Broadcast_method_obj, 1, ble_Broadcast_method);

// dir(pybricks.ble.Broadcast)
STATIC const mp_rom_map_elem_t ble_Broadcast_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_method),       MP_ROM_PTR(&ble_Broadcast_method_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ble_Broadcast_locals_dict, ble_Broadcast_locals_dict_table);

// type(pybricks.ble.Broadcast)
const mp_obj_type_t pb_type_Broadcast = {
    { &mp_type_type },
    .name = MP_QSTR_Broadcast,
    .make_new = ble_Broadcast_make_new,
    .locals_dict = (mp_obj_dict_t *)&ble_Broadcast_locals_dict,
};

#endif // PYBRICKS_PY_BLE
