// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_CHARGER

#include <pbdrv/charger.h>
#include <pbdrv/usb.h>

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct {
    mp_obj_base_t base;
} pb_obj_Charger_t;

static mp_obj_t Charger_current(mp_obj_t self_in) {
    uint16_t current;
    pb_assert(pbdrv_charger_get_current_now(&current));
    return mp_obj_new_int(current);
}
static MP_DEFINE_CONST_FUN_OBJ_1(Charger_current_obj, Charger_current);

static mp_obj_t Charger_status(mp_obj_t self_in) {
    return MP_OBJ_NEW_SMALL_INT(pbdrv_charger_get_status());
}
static MP_DEFINE_CONST_FUN_OBJ_1(Charger_status_obj, Charger_status);

static mp_obj_t Charger_connected(mp_obj_t self_in) {
    return mp_obj_new_int(pbdrv_usb_get_bcd());
}
static MP_DEFINE_CONST_FUN_OBJ_1(Charger_connected_obj, Charger_connected);

static const mp_rom_map_elem_t Charger_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_connected), MP_ROM_PTR(&Charger_connected_obj) },
    { MP_ROM_QSTR(MP_QSTR_current), MP_ROM_PTR(&Charger_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&Charger_status_obj) },
};
static MP_DEFINE_CONST_DICT(Charger_locals_dict, Charger_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_Charger,
    MP_QSTR_Charger,
    MP_TYPE_FLAG_NONE,
    locals_dict, &Charger_locals_dict);

mp_obj_t pb_type_Charger_obj_new(void) {
    pb_obj_Charger_t *self = mp_obj_malloc(pb_obj_Charger_t, &pb_type_Charger);
    return MP_OBJ_FROM_PTR(self);
}

#endif // PYBRICKS_PY_COMMON_CHARGER
