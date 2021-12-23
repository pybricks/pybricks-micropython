// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_PRIMEHUB

#if !PYBRICKS_PY_COMMON_LIGHT_MATRIX || !PYBRICKS_PY_PARAMETERS_BUTTON
#error "PYBRICKS_PY_COMMON_LIGHT_MATRIX and PYBRICKS_PY_PARAMETERS_BUTTON must be enabled."
#endif

#include <pbsys/light.h>
#include <pbio/util.h>

#include "py/runtime.h"
#include "py/obj.h"

#include <pbio/button.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/common.h>
#include <pybricks/geometry.h>
#include <pybricks/hubs.h>

typedef struct _hubs_PrimeHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t buttons;
    mp_obj_t charger;
    mp_obj_t display;
    mp_obj_t imu;
    mp_obj_t light;
    mp_obj_t speaker;
} hubs_PrimeHub_obj_t;

static const pb_obj_enum_member_t *primehub_buttons[] = {
    &pb_Button_LEFT_obj,
    &pb_Button_RIGHT_obj,
    &pb_Button_BLUETOOTH_obj,
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_PrimeHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_OBJ(top_side, pb_Axis_Z_obj),
        PB_ARG_DEFAULT_OBJ(front_side, pb_Axis_X_obj));

    hubs_PrimeHub_obj_t *self = m_new_obj(hubs_PrimeHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->buttons = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(primehub_buttons), primehub_buttons, pbio_button_is_pressed);
    self->charger = pb_type_Charger_obj_new();
    self->display = pb_type_Lightmatrix_obj_new(pbsys_hub_light_matrix);
    self->imu = pb_type_IMU_obj_new(top_side_in, front_side_in);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    self->speaker = mp_call_function_0(MP_OBJ_FROM_PTR(&pb_type_Speaker));
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t hubs_PrimeHub_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)},
    { MP_ROM_QSTR(MP_QSTR_system),      MP_ROM_PTR(&pb_type_System)   },
};
STATIC MP_DEFINE_CONST_DICT(hubs_PrimeHub_locals_dict, hubs_PrimeHub_locals_dict_table);

STATIC const pb_attr_dict_entry_t hubs_PrimeHub_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_buttons, hubs_PrimeHub_obj_t, buttons),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_charger, hubs_PrimeHub_obj_t, charger),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_display, hubs_PrimeHub_obj_t, display),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_imu, hubs_PrimeHub_obj_t, imu),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_light, hubs_PrimeHub_obj_t, light),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_speaker, hubs_PrimeHub_obj_t, speaker),
};

const pb_obj_with_attr_type_t pb_type_ThisHub = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = PYBRICKS_HUB_CLASS_NAME,
        .make_new = hubs_PrimeHub_make_new,
        .attr = pb_attribute_handler,
        .locals_dict = (mp_obj_dict_t *)&hubs_PrimeHub_locals_dict,
    },
    .attr_dict = hubs_PrimeHub_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(hubs_PrimeHub_attr_dict),
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_PRIMEHUB
