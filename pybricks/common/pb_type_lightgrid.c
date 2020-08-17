// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_LIGHTGRID

#include <pbdrv/pwm.h>

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"

#include <pybricks/common.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

// FIXME: pbio will have a light grid interface. Use that when ready.
static uint8_t channels[5][5] = {
    {38, 36, 41, 46, 33},
    {37, 28, 39, 47, 21},
    {24, 29, 31, 45, 23},
    {26, 27, 32, 34, 22},
    {25, 40, 30, 35, 9 },
};

// FIXME: pbio will have a light grid interface. Use that when ready.
static void set_light_grid_full(pbdrv_pwm_dev_t *pwm, const uint8_t *image) {
    for (uint8_t i = 0; i < 5; i++) {
        for (uint8_t j = 0; j < 5; j++) {
            pb_assert(pwm->funcs->set_duty(pwm, channels[i][j], image[i] & (1 << (4 - j)) ? UINT16_MAX: 0));
        }
    }
}

// FIXME: pbio will have a light grid interface. Use that when ready.
static void set_pixel_brightness(pbdrv_pwm_dev_t *pwm, uint8_t row, uint8_t col, int32_t brightness) {
    pb_assert(pwm->funcs->set_duty(pwm, channels[row][col], brightness * brightness * UINT16_MAX / 10000));
}

// pybricks._common.LightGrid class object
typedef struct _common_LightGrid_obj_t {
    mp_obj_base_t base;
    pbdrv_pwm_dev_t *pwm;
    uint8_t size;
} common_LightGrid_obj_t;

// pybricks._common.LightGrid.char
STATIC mp_obj_t common_LightGrid_char(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(character));

    GET_STR_DATA_LEN(character, str, len);
    if (len != 1 || str[0] < 32 || str[0] > 126) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }
    set_light_grid_full(self->pwm, pb_font_5x5[str[0] - 32]);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_char_obj, 1, common_LightGrid_char);

// pybricks._common.LightGrid.on
STATIC mp_obj_t common_LightGrid_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    common_LightGrid_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    set_light_grid_full(self->pwm, pb_font_5x5['H' - 32]);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_on_obj, 1, common_LightGrid_on);

// pybricks._common.LightGrid.off
STATIC mp_obj_t common_LightGrid_off(mp_obj_t self_in) {
    common_LightGrid_obj_t *self = MP_OBJ_TO_PTR(self_in);

    set_light_grid_full(self->pwm, pb_font_5x5[' ' - 32]);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_LightGrid_off_obj, common_LightGrid_off);

// pybricks._common.LightGrid.number
STATIC mp_obj_t common_LightGrid_number(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(number));

    // Get the number
    mp_int_t value = pb_obj_get_int(number);

    // > 99 gets displayed as >
    if (value > 99) {
        set_light_grid_full(self->pwm, pb_font_5x5['>' - 32]);
        return mp_const_none;
    }

    // < -99 gets displayed as <
    if (value < -99) {
        set_light_grid_full(self->pwm, pb_font_5x5['<' - 32]);
        return mp_const_none;
    }

    // Remember sign but make value positive
    bool negative = value < 0;
    if (negative) {
        value = -value;
    }

    // Compose number as two digits
    uint8_t composite[5];
    for (uint8_t i = 0; i < 5; i++) {
        composite[i] = pb_digits_5x2[value / 10][i] << 3 | pb_digits_5x2[value % 10][i];
    }

    // Display one dot to indicate negative
    // FIXME: Display faintly to distinguish from digits
    if (negative) {
        composite[3] |= 0b00100;
    }

    // Display the result
    set_light_grid_full(self->pwm, composite);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_number_obj, 1, common_LightGrid_number);

// pybricks._common.LightGrid.pixel
STATIC mp_obj_t common_LightGrid_pixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(row),
        PB_ARG_REQUIRED(column),
        PB_ARG_DEFAULT_INT(brightness, 100));

    // Get the position
    mp_int_t r = mp_obj_get_int(row);
    mp_int_t c = mp_obj_get_int(column);
    if (r < 0 || r > self->size || c < 0 || c > self->size) {
        // Print outsize of canvas, so return
        return mp_const_none;
    }

    // Get the brightness
    mp_int_t b = pb_obj_get_pct(brightness);

    // Set pixel
    set_pixel_brightness(self->pwm, r, c, b);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_pixel_obj, 1, common_LightGrid_pixel);

// dir(pybricks.builtins.LightGrid)
STATIC const mp_rom_map_elem_t common_LightGrid_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_char),   MP_ROM_PTR(&common_LightGrid_char_obj)   },
    { MP_ROM_QSTR(MP_QSTR_on),     MP_ROM_PTR(&common_LightGrid_on_obj)     },
    { MP_ROM_QSTR(MP_QSTR_off),    MP_ROM_PTR(&common_LightGrid_off_obj)    },
    { MP_ROM_QSTR(MP_QSTR_number), MP_ROM_PTR(&common_LightGrid_number_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel),  MP_ROM_PTR(&common_LightGrid_pixel_obj)  },
};
STATIC MP_DEFINE_CONST_DICT(common_LightGrid_locals_dict, common_LightGrid_locals_dict_table);

// type(pybricks.builtins.LightGrid)
STATIC const mp_obj_type_t pb_type_LightGrid = {
    { &mp_type_type },
    .name = MP_QSTR_LightGrid,
    .locals_dict = (mp_obj_dict_t *)&common_LightGrid_locals_dict,
};

// pybricks._common.LightGrid.__init__
mp_obj_t common_LightGrid_obj_make_new(uint8_t size) {
    // Create new light instance
    common_LightGrid_obj_t *grid = m_new_obj(common_LightGrid_obj_t);
    // Set type and iodev
    grid->base.type = &pb_type_LightGrid;
    grid->size = size;
    pb_assert(pbdrv_pwm_get_dev(4, &grid->pwm));
    return grid;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_LIGHTGRID
