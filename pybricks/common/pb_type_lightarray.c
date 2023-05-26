// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_LIGHT_ARRAY

#include "py/obj.h"

#include <pybricks/common.h>

#include <pybricks/pupdevices.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

// pybricks._common.Light class object
typedef struct _common_LightArray_obj_t {
    mp_obj_base_t base;
    pb_pupdevices_obj_base_t *sensor;
    uint8_t light_mode;
    uint8_t number_of_lights;
} common_LightArray_obj_t;

// pybricks._common.LightArray.on
STATIC mp_obj_t common_LightArray_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightArray_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    int8_t brightness_values[4];

    // Given an integer, make all lights the same brightness.
    if (mp_obj_is_int(brightness_in)) {
        int32_t b = pb_obj_get_pct(brightness_in);
        for (uint8_t i = 0; i < self->number_of_lights; i++) {
            brightness_values[i] = b;
        }
    }
    // Otherwise, get each brightness value from list or tuple.
    else {
        mp_obj_t *brightness_objects;
        size_t num_values;
        mp_obj_get_array(brightness_in, &num_values, &brightness_objects);
        if (num_values != self->number_of_lights) {
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
        for (uint8_t i = 0; i < self->number_of_lights; i++) {
            brightness_values[i] = pb_obj_get_pct(brightness_objects[i]);
        }
    }

    // Set the brightness values and wait or await it.
    return pb_pupdevices_set_data(self->sensor, self->light_mode, brightness_values);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightArray_on_obj, 1, common_LightArray_on);

// pybricks._common.LightArray.off
STATIC mp_obj_t common_LightArray_off(mp_obj_t self_in) {
    const mp_obj_t pos_args[] = {self_in, MP_OBJ_NEW_SMALL_INT(0) };
    common_LightArray_on(MP_ARRAY_SIZE(pos_args), pos_args, NULL);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_LightArray_off_obj, common_LightArray_off);

// dir(pybricks.builtins.LightArray)
STATIC const mp_rom_map_elem_t common_LightArray_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&common_LightArray_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&common_LightArray_off_obj) },
};
STATIC MP_DEFINE_CONST_DICT(common_LightArray_locals_dict, common_LightArray_locals_dict_table);

// type(pybricks.builtins.LightArray)
STATIC MP_DEFINE_CONST_OBJ_TYPE(pb_type_LightArray,
    MP_QSTR_LightArray,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_LightArray_locals_dict);

// pybricks._common.LightArray.__init__
mp_obj_t common_LightArray_obj_make_new(pb_pupdevices_obj_base_t *sensor, uint8_t light_mode, uint8_t number_of_lights) {
    common_LightArray_obj_t *light = mp_obj_malloc(common_LightArray_obj_t, &pb_type_LightArray);
    light->sensor = sensor;
    light->light_mode = light_mode;
    light->number_of_lights = number_of_lights;
    return MP_OBJ_FROM_PTR(light);
}

#endif // PYBRICKS_PY_COMMON_LIGHT_ARRAY
