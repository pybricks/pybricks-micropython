// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include <pbio/light.h>
#include <pbio/color.h>

#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

// pybricks._common.ColorLight class object
typedef struct {
    mp_obj_base_t base;
    void *context;
    pb_type_ColorLight_on_t on;

} common_ColorLight_external_obj_t;

// pybricks._common.ColorLight.on
STATIC mp_obj_t common_ColorLight_external_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse arguments
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_ColorLight_external_obj_t, self,
        PB_ARG_REQUIRED(color));

    return self->on(self->context, pb_type_Color_get_hsv(color_in));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_ColorLight_external_on_obj, 1, common_ColorLight_external_on);

// pybricks._common.ColorLight.off
STATIC mp_obj_t common_ColorLight_external_off(mp_obj_t self_in) {
    common_ColorLight_external_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->on(self->context, &pb_Color_NONE_obj.hsv);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_ColorLight_external_off_obj, common_ColorLight_external_off);

// dir(pybricks.builtins.ColorLight)
STATIC const mp_rom_map_elem_t common_ColorLight_external_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&common_ColorLight_external_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&common_ColorLight_external_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(common_ColorLight_external_locals_dict, common_ColorLight_external_locals_dict_table);

// type(pybricks.builtins.ColorLight)
STATIC MP_DEFINE_CONST_OBJ_TYPE(pb_type_ColorLight_external,
    MP_QSTR_ColorLight,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_ColorLight_external_locals_dict);

// pybricks._common.ColorLight.__init__
mp_obj_t pb_type_ColorLight_external_obj_new(void *context, pb_type_ColorLight_on_t on) {
    common_ColorLight_external_obj_t *light = mp_obj_malloc(common_ColorLight_external_obj_t, &pb_type_ColorLight_external);
    light->context = context;
    light->on = on;
    return MP_OBJ_FROM_PTR(light);
}

#endif // PYBRICKS_PY_COMMON
