// SPDX-License-Identifier: MIT
// Copyright (c) 2022-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_VIRTUALHUB

#include <pbio/button.h>
#include <pbsys/light.h>

#include "py/misc.h"
#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_pb/pb_error.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>

#include <pybricks/common.h>
#include <pybricks/hubs.h>
#include <pybricks/parameters.h>

typedef struct _hubs_VirtualHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t battery;
    mp_obj_t buttons;
    mp_obj_t light;
    mp_obj_t screen;
    mp_obj_t system;
    #if PYBRICKS_PY_COMMON_BLE
    mp_obj_t ble;
    #endif
} hubs_VirtualHub_obj_t;

static mp_obj_t hubs_VirtualHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_OBJ(top_side, pb_type_Axis_Z_obj),
        PB_ARG_DEFAULT_OBJ(front_side, pb_type_Axis_X_obj)
        #if PYBRICKS_PY_COMMON_BLE
        , PB_ARG_DEFAULT_NONE(broadcast_channel)
        , PB_ARG_DEFAULT_OBJ(observe_channels, mp_const_empty_tuple_obj)
        #endif
        );

    (void)top_side_in;
    (void)front_side_in;



    hubs_VirtualHub_obj_t *self = mp_obj_malloc(hubs_VirtualHub_obj_t, type);
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);

    #if PYBRICKS_PY_COMMON_BLE
    self->ble = pb_type_BLE_new(broadcast_channel_in, observe_channels_in);
    #endif

    self->buttons = pb_type_Keypad_obj_new(MP_OBJ_FROM_PTR(self), pb_type_button_pressed_hub_single_button);
    // FIXME: Implement lights.
    // self->light = common_ColorLight_internal_obj_new(pbsys_status_light_main);
    self->screen = pb_type_Image_display_obj_new();
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

static const pb_attr_dict_entry_t hubs_VirtualHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_VirtualHub_obj_t, battery),
    #if PYBRICKS_PY_COMMON_BLE
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_ble, hubs_VirtualHub_obj_t, ble),
    #endif
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, hubs_VirtualHub_obj_t, buttons),
    // PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_VirtualHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_screen, hubs_VirtualHub_obj_t, screen),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_VirtualHub_obj_t, system),
    PB_ATTR_DICT_SENTINEL
};

MP_DEFINE_CONST_OBJ_TYPE(pb_type_ThisHub,
    PYBRICKS_HUB_CLASS_NAME,
    MP_TYPE_FLAG_NONE,
    make_new, hubs_VirtualHub_make_new,
    attr, pb_attribute_handler,
    protocol, hubs_VirtualHub_attr_dict);

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_VIRTUALHUB
