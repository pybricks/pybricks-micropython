// SPDX-License-Identifier: MIT
// Copyright (c) 2021-2022 The Pybricks Authors

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


STATIC mp_obj_t Charger_current(mp_obj_t self_in) {
    uint16_t current;
    pb_assert(pbdrv_charger_get_current_now(&current));
    return mp_obj_new_int(current);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Charger_current_obj, Charger_current);

STATIC mp_obj_t Charger_status(mp_obj_t self_in) {
    pbdrv_charger_status_t status = pbdrv_charger_get_status();

    switch (status) {
        case PBDRV_CHARGER_STATUS_DISCHARGE:
            return MP_OBJ_NEW_QSTR(MP_QSTR_discharge);
        case PBDRV_CHARGER_STATUS_CHARGE:
            return MP_OBJ_NEW_QSTR(MP_QSTR_charge);
        case PBDRV_CHARGER_STATUS_COMPLETE:
            return MP_OBJ_NEW_QSTR(MP_QSTR_complete);
        case PBDRV_CHARGER_STATUS_FAULT:
            return MP_OBJ_NEW_QSTR(MP_QSTR_fault);
        default:
            mp_raise_NotImplementedError(MP_ERROR_TEXT("unknown"));
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Charger_status_obj, Charger_status);

STATIC mp_obj_t Charger_usb(mp_obj_t self_in) {
    switch (pbdrv_usb_get_bcd()) {
        case PBDRV_USB_BCD_NONE:
            return mp_const_none;
        case PBDRV_USB_BCD_NONSTANDARD:
            return MP_OBJ_NEW_QSTR(MP_QSTR_nonstandard);
        case PBDRV_USB_BCD_STANDARD_DOWNSTREAM:
            return MP_OBJ_NEW_QSTR(MP_QSTR_standard);
        case PBDRV_USB_BCD_CHARGING_DOWNSTREAM:
            return MP_OBJ_NEW_QSTR(MP_QSTR_charging);
        case PBDRV_USB_BCD_DEDICATED_CHARGING:
            return MP_OBJ_NEW_QSTR(MP_QSTR_dedicated);
        default:
            mp_raise_NotImplementedError(MP_ERROR_TEXT("unknown"));
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(Charger_usb_obj, Charger_usb);

STATIC const mp_rom_map_elem_t Charger_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_current), MP_ROM_PTR(&Charger_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&Charger_status_obj) },
    { MP_ROM_QSTR(MP_QSTR_usb), MP_ROM_PTR(&Charger_usb_obj) },
};
STATIC MP_DEFINE_CONST_DICT(Charger_locals_dict, Charger_locals_dict_table);

const mp_obj_type_t pb_type_Charger = {
    .base = { .type = &mp_type_type },
    .name = MP_QSTR_Charger,
    .locals_dict = (mp_obj_dict_t *)&Charger_locals_dict,
};

mp_obj_t pb_type_Charger_obj_new(void) {
    pb_obj_Charger_t *self = m_new_obj(pb_obj_Charger_t);
    self->base.type = &pb_type_Charger;
    return MP_OBJ_FROM_PTR(self);
}

#endif // PYBRICKS_PY_COMMON_CHARGER
