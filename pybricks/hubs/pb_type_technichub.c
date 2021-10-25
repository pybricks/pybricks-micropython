// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_HUBS && PYBRICKS_HUB_TECHNICHUB

#include <pbdrv/reset.h>
#include <pbsys/light.h>

#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include <pybricks/common.h>
#include <pybricks/geometry.h>
#include <pybricks/hubs.h>

typedef struct _hubs_TechnicHub_obj_t {
    mp_obj_base_t base;
    mp_obj_t button;
    mp_obj_t imu;
    mp_obj_t light;
} hubs_TechnicHub_obj_t;

static const pb_obj_enum_member_t *technichub_buttons[] = {
    &pb_Button_CENTER_obj,
};

STATIC mp_obj_t hubs_TechnicHub_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_DEFAULT_OBJ(top_side, pb_Axis_Z_obj),
        PB_ARG_DEFAULT_OBJ(front_side, pb_Axis_X_obj));

    hubs_TechnicHub_obj_t *self = m_new_obj(hubs_TechnicHub_obj_t);
    self->base.type = (mp_obj_type_t *)type;
    self->button = pb_type_Keypad_obj_new(MP_ARRAY_SIZE(technichub_buttons), technichub_buttons, pbio_button_is_pressed);
    self->imu = pb_type_IMU_obj_new(top_side_in, front_side_in);
    self->light = common_ColorLight_internal_obj_new(pbsys_status_light);
    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_rom_map_elem_t attribute_table[] = {
    PB_DEFINE_CONST_ATTR_RO(hubs_TechnicHub_obj_t, MP_QSTR_button, button),
    PB_DEFINE_CONST_ATTR_RO(hubs_TechnicHub_obj_t, MP_QSTR_imu, imu),
    PB_DEFINE_CONST_ATTR_RO(hubs_TechnicHub_obj_t, MP_QSTR_light, light),
};
STATIC MP_DEFINE_CONST_DICT(attribute_dict, attribute_table);

STATIC const mp_rom_map_elem_t hubs_TechnicHub_locals_dict_table[] = {
    PB_ATTRIBUTE_TABLE(attribute_dict),
    { MP_ROM_QSTR(MP_QSTR_battery),     MP_ROM_PTR(&pb_module_battery)    },
    { MP_ROM_QSTR(MP_QSTR_system),      MP_ROM_PTR(&pb_type_System)                           },
};
STATIC MP_DEFINE_CONST_DICT(hubs_TechnicHub_locals_dict, hubs_TechnicHub_locals_dict_table);

const mp_obj_type_t pb_type_ThisHub = {
    { &mp_type_type },
    .name = PYBRICKS_HUB_CLASS_NAME,
    .make_new = hubs_TechnicHub_make_new,
    .attr = pb_attribute_handler,
    .locals_dict = (mp_obj_dict_t *)&hubs_TechnicHub_locals_dict,
};

#endif // PYBRICKS_PY_HUBS && PYBRICKS_HUB_TECHNICHUB
