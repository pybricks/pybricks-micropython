// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_LIGHT_ARRAY

#include "py/obj.h"

#include <pybricks/common.h>

#include <pybricks/pupdevices.h>
#include <pybricks/common/pb_type_device.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

// pybricks._common.Light class object
typedef struct _common_LightArray_obj_t {
    mp_obj_base_t base;
    pb_type_device_obj_base_t *sensor;
    uint8_t light_mode;
    uint8_t number_of_lights;
} common_LightArray_obj_t;

// pybricks._common.LightArray.on
static mp_obj_t common_LightArray_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightArray_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    int8_t brightness_values[4];
    pb_obj_get_pct_or_array(brightness_in, self->number_of_lights, brightness_values);

    // Set the brightness values and wait or await it.
    return pb_type_device_set_data(self->sensor, self->light_mode, brightness_values, self->number_of_lights);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightArray_on_obj, 1, common_LightArray_on);

// pybricks._common.LightArray.off
static mp_obj_t common_LightArray_off(mp_obj_t self_in) {
    common_LightArray_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t brightness_values[4] = { };
    return pb_type_device_set_data(self->sensor, self->light_mode, brightness_values, self->number_of_lights);
}
static MP_DEFINE_CONST_FUN_OBJ_1(common_LightArray_off_obj, common_LightArray_off);

// dir(pybricks.builtins.LightArray)
static const mp_rom_map_elem_t common_LightArray_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&common_LightArray_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&common_LightArray_off_obj) },
};
static MP_DEFINE_CONST_DICT(common_LightArray_locals_dict, common_LightArray_locals_dict_table);

// type(pybricks.builtins.LightArray)
static MP_DEFINE_CONST_OBJ_TYPE(pb_type_LightArray,
    MP_QSTRnull,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_LightArray_locals_dict);

// pybricks._common.LightArray.__init__
mp_obj_t common_LightArray_obj_make_new(pb_type_device_obj_base_t *sensor, uint8_t light_mode, uint8_t number_of_lights) {
    common_LightArray_obj_t *light = mp_obj_malloc(common_LightArray_obj_t, &pb_type_LightArray);
    light->sensor = sensor;
    light->light_mode = light_mode;
    light->number_of_lights = number_of_lights;
    return MP_OBJ_FROM_PTR(light);
}

#endif // PYBRICKS_PY_COMMON_LIGHT_ARRAY
