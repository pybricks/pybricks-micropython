// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_CITYHUB

#include <pbdrv/reset.h>
#include <pbsys/light.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

typedef struct _hubs_CityHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t battery;
    #if PYBRICKS_PY_COMMON_BLE
    mp_obj_t ble;
    #endif
    mp_obj_t button;
    mp_obj_t light;
    mp_obj_t system;
} hubs_CityHub_obj_t;

static const pb_obj_enum_member_t *cityhub_buttons[] = {
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_CityHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    #if PYBRICKS_PY_COMMON_BLE
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_INT(broadcast_channel, 0),
        PB_ARG_DEFAULT_OBJ(observe_channels, mp_const_empty_tuple_obj));
    #endif

    hubs_CityHub_obj_t *self = mp_obj_malloc(hubs_CityHub_obj_t, type);
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    #if PYBRICKS_PY_COMMON_BLE
    self->ble = pb_type_BLE_new(broadcast_channel_in, observe_channels_in);
    #endif
    self->button = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(cityhub_buttons), cityhub_buttons, pbio_button_is_pressed);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const pb_attr_dict_entry_t hubs_CityHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_CityHub_obj_t, battery),
    #if PYBRICKS_PY_COMMON_BLE
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_ble, hubs_CityHub_obj_t, ble),
    #endif
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_button, hubs_CityHub_obj_t, button),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_CityHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_CityHub_obj_t, system),
    PB_ATTR_DICT_SENTINEL
};

MP_DEFINE_CONST_OBJ_TYPE(pb_type_ThisHub,
    MP_QSTR_CityHub,
    MP_TYPE_FLAG_NONE,
    make_new, hubs_CityHub_make_new,
    attr, pb_attribute_handler,
    protocol, hubs_CityHub_attr_dict);

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_CITYHUB
