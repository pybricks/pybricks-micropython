// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_TECHNICHUB

#include <pbdrv/reset.h>
#include <pbsys/light.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include <pybricks/common.h>
#include <pybricks/tools/pb_type_matrix.h>
#include <pybricks/hubs.h>

typedef struct _hubs_TechnicHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t battery;
    #if PYBRICKS_PY_COMMON_BLE
    mp_obj_t ble;
    #endif
    mp_obj_t button;
    mp_obj_t imu;
    mp_obj_t light;
    mp_obj_t system;
} hubs_TechnicHub_obj_t;

static mp_obj_t hubs_TechnicHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_OBJ(top_side, pb_type_Axis_Z_obj),
        PB_ARG_DEFAULT_OBJ(front_side, pb_type_Axis_X_obj)
        #if PYBRICKS_PY_COMMON_BLE
        , PB_ARG_DEFAULT_INT(broadcast_channel, 0)
        , PB_ARG_DEFAULT_OBJ(observe_channels, mp_const_empty_tuple_obj)
        #endif
        );

    hubs_TechnicHub_obj_t *self = mp_obj_malloc(hubs_TechnicHub_obj_t, type);
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    #if PYBRICKS_PY_COMMON_BLE
    self->ble = pb_type_BLE_new(broadcast_channel_in, observe_channels_in);
    #endif
    self->button = pb_type_Keypad_obj_new(pb_type_button_pressed_hub_single_button);
    self->imu = pb_type_IMU_obj_new(MP_OBJ_FROM_PTR(self), top_side_in, front_side_in);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

static const pb_attr_dict_entry_t hubs_TechnicHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_TechnicHub_obj_t, battery),
    #if PYBRICKS_PY_COMMON_BLE
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_ble, hubs_TechnicHub_obj_t, ble),
    #endif
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, hubs_TechnicHub_obj_t, button),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_button, hubs_TechnicHub_obj_t, button), // backwards compatibility
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_imu, hubs_TechnicHub_obj_t, imu),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_TechnicHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_TechnicHub_obj_t, system),
    PB_ATTR_DICT_SENTINEL
};

MP_DEFINE_CONST_OBJ_TYPE(pb_type_ThisHub,
    PYBRICKS_HUB_CLASS_NAME,
    MP_TYPE_FLAG_NONE,
    make_new, hubs_TechnicHub_make_new,
    attr, pb_attribute_handler,
    protocol, hubs_TechnicHub_attr_dict);

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_TECHNICHUB
