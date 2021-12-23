// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_ESSENTIALHUB

#if !PYBRICKS_PY_PARAMETERS_BUTTON
#error "PYBRICKS_PY_PARAMETERS_BUTTON must be enabled."
#endif

#include <pbsys/light.h>
#include <pbio/util.h>

#include "py/runtime.h"
#include "py/obj.h"

#include <pbio/button.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include <pybricks/common.h>
#include <pybricks/geometry.h>
#include <pybricks/hubs.h>

typedef struct _hubs_EssentialHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t battery;
    mp_obj_t buttons;
    mp_obj_t charger;
    mp_obj_t imu;
    mp_obj_t light;
    mp_obj_t system;
} hubs_EssentialHub_obj_t;

static const pb_obj_enum_member_t *essentialhub_buttons[] = {
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_EssentialHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_OBJ(top_side, pb_Axis_Z_obj),
        PB_ARG_DEFAULT_OBJ(front_side, pb_Axis_X_obj));

    hubs_EssentialHub_obj_t *self = m_new_obj(hubs_EssentialHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->battery = MP_OBJ_FROM_PTR(&pb_module_battery);
    self->buttons = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(essentialhub_buttons), essentialhub_buttons, pbio_button_is_pressed);
    self->charger = pb_type_Charger_obj_new();
    self->imu = pb_type_IMU_obj_new(top_side_in, front_side_in);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->system = MP_OBJ_FROM_PTR(&pb_type_System);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const pb_attr_dict_entry_t hubs_EssentialHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_battery, hubs_EssentialHub_obj_t, battery),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_button, hubs_EssentialHub_obj_t, buttons),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_charger, hubs_EssentialHub_obj_t, charger),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_imu, hubs_EssentialHub_obj_t, imu),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_EssentialHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_system, hubs_EssentialHub_obj_t, system),
};

const pb_obj_with_attr_type_t pb_type_ThisHub = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = PYBRICKS_HUB_CLASS_NAME,
        .make_new = hubs_EssentialHub_make_new,
        .attr = pb_attribute_handler,
    },
    .attr_dict = hubs_EssentialHub_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(hubs_EssentialHub_attr_dict),
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_ESSENTIALHUB
