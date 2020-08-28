// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON

#include <stdbool.h>

#include <pbio/light.h>
#include <pbio/color.h>

#include "py/misc.h"
#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/common.h>
#include <pybricks/parameters.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

// pybricks._common.ColorLight class object
typedef struct _common_ColorLight_internal_obj_t {
    mp_obj_base_t base;
    pbio_color_light_t *light;
    void *animation_cells;
    #if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    size_t cells_size;
    #endif
} common_ColorLight_internal_obj_t;

// pybricks._common.ColorLight.on
STATIC mp_obj_t common_ColorLight_internal_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse arguments
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_ColorLight_internal_obj_t, self,
        PB_ARG_REQUIRED(color));

    pb_assert(pbio_color_light_on_hsv(self->light, pb_type_Color_get_hsv(color_in)));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_ColorLight_internal_on_obj, 1, common_ColorLight_internal_on);

// pybricks._common.ColorLight.off
STATIC mp_obj_t common_ColorLight_internal_off(mp_obj_t self_in) {
    common_ColorLight_internal_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_assert(pbio_color_light_off(self->light));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_ColorLight_internal_off_obj, common_ColorLight_internal_off);

// pybricks._common.ColorLight.blink
STATIC mp_obj_t common_ColorLight_internal_blink(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_ColorLight_internal_obj_t, self,
        PB_ARG_REQUIRED(color),
        PB_ARG_REQUIRED(durations));

    pbio_color_t color = pb_type_enum_get_value(color_in, &pb_enum_type_Color);

    mp_int_t durations_len = mp_obj_get_int(mp_obj_len(durations_in));

    size_t cells_size = sizeof(pbio_color_light_blink_cell_t) * (durations_len + 1);
    #if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    self->animation_cells = m_realloc(self->animation_cells, self->cells_size, cells_size);
    self->cells_size = cells_size;
    #else
    self->animation_cells = m_realloc(self->animation_cells, cells_size);
    #endif

    pbio_color_light_blink_cell_t *cells = self->animation_cells;
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t durations_iter = mp_getiter(durations_in, &iter_buf);
    for (int i = 0; i < durations_len; i++) {
        cells[i].color = i % 2 ? PBIO_COLOR_BLACK : color;
        cells[i].duration = pb_obj_get_positive_int(mp_iternext(durations_iter));
    }

    // sentinel value
    cells[durations_len].duration = 0;

    pbio_color_light_start_blink_animation(self->light, cells);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_ColorLight_internal_blink_obj, 1, common_ColorLight_internal_blink);

// pybricks._common.ColorLight.animate
STATIC mp_obj_t common_ColorLight_internal_animate(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_ColorLight_internal_obj_t, self,
        PB_ARG_REQUIRED(colors),
        PB_ARG_REQUIRED(interval));

    mp_int_t colors_len = mp_obj_get_int(mp_obj_len(colors_in));

    size_t cells_size = sizeof(pbio_color_hsv_t) * (colors_len + 1);
    #if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    self->animation_cells = m_realloc(self->animation_cells, self->cells_size, cells_size);
    self->cells_size = cells_size;
    #else
    self->animation_cells = m_realloc(self->animation_cells, cells_size);
    #endif

    pbio_color_hsv_t *cells = self->animation_cells;
    mp_obj_iter_buf_t iter_buf;
    mp_obj_t colors_iter = mp_getiter(colors_in, &iter_buf);
    for (int i = 0; i < colors_len; i++) {
        pb_obj_get_hsv(mp_iternext(colors_iter), &cells[i]);
    }

    // sentinel value
    cells[colors_len].h = UINT16_MAX;

    mp_int_t interval = pb_obj_get_int(interval_in);

    pbio_color_light_start_animation(self->light, interval, cells);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_ColorLight_internal_animate_obj, 1, common_ColorLight_internal_animate);

// dir(pybricks.builtins.ColorLight)
STATIC const mp_rom_map_elem_t common_ColorLight_internal_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&common_ColorLight_internal_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&common_ColorLight_internal_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_blink), MP_ROM_PTR(&common_ColorLight_internal_blink_obj) },
    { MP_ROM_QSTR(MP_QSTR_animate), MP_ROM_PTR(&common_ColorLight_internal_animate_obj) },
};
STATIC MP_DEFINE_CONST_DICT(common_ColorLight_internal_locals_dict, common_ColorLight_internal_locals_dict_table);

// type(pybricks.builtins.ColorLight)
STATIC const mp_obj_type_t pb_type_ColorLight_internal = {
    { &mp_type_type },
    .name = MP_QSTR_ColorLight,
    .locals_dict = (mp_obj_dict_t *)&common_ColorLight_internal_locals_dict,
};

// pybricks._common.ColorLight.__init__
mp_obj_t common_ColorLight_internal_obj_new(pbio_color_light_t *light) {
    // Create new light instance
    common_ColorLight_internal_obj_t *self = m_new_obj(common_ColorLight_internal_obj_t);

    self->base.type = &pb_type_ColorLight_internal;
    self->light = light;
    #if MICROPY_MALLOC_USES_ALLOCATED_SIZE
    self->cells_size = 0;
    #endif

    return self;
}

#endif // PYBRICKS_PY_COMMON
