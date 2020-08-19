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

static uint8_t spike_prime_tlc5955_pwm_channel_map[] = {
    38, 36, 41, 46, 33,
    37, 28, 39, 47, 21,
    24, 29, 31, 45, 23,
    26, 27, 32, 34, 22,
    25, 40, 30, 35, 9,
};

// pybricks._common.LightGrid class object
typedef struct _common_LightGrid_obj_t {
    mp_obj_base_t base;
    pbdrv_pwm_dev_t *pwm;
    uint8_t size;
    uint8_t *channels;
} common_LightGrid_obj_t;

// Each byte sets a row, where 1 means a pixel is on and 0 is off. Least significant bit is on the right.
// This is the same format as used by the micro:bit.
static void light_grid_set_rows_binary(common_LightGrid_obj_t *self, const uint8_t *rows) {
    // Loop through all rows i, starting at row 0 at the top.
    for (uint8_t i = 0; i < self->size; i++) {
        // Loop through all columns j, starting at col 0 on the left.
        for (uint8_t j = 0; j < self->size; j++) {
            // The pixel is on of the bit is high.
            bool on = rows[i] & (1 << (self->size - 1 - j));
            // Set the pixel.
            pb_assert(
                self->pwm->funcs->set_duty(self->pwm, self->channels[i * self->size + j], on * UINT16_MAX)
                );
        }
    }
}

// Sets one pixel to an approximately perceived brightness of 0--100%
static void set_pixel_brightness(common_LightGrid_obj_t *self, uint8_t row, uint8_t col, int32_t brightness) {

    // Return if the requested pixel is out of bounds
    if (row >= self->size || col >= self->size) {
        return;
    }

    // Scale brightness quadratically from 0 to UINT16_MAX
    int32_t duty = brightness * brightness * UINT16_MAX / 10000;

    // Set the duty cycle for this pixel
    pb_assert(
        self->pwm->funcs->set_duty(
            self->pwm, self->channels[row * self->size + col], duty
            )
        );
}

// pybricks._common.LightGrid.char
STATIC mp_obj_t common_LightGrid_char(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(character));

    // Currently characters are only implemented for 5x5 grids
    if (self->size != 5) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    // Assert that the input is a single character
    GET_STR_DATA_LEN(character, str, len);
    if (len != 1 || str[0] < 32 || str[0] > 126) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Pick corresponding image and display it
    light_grid_set_rows_binary(self, pb_font_5x5[str[0] - 32]);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_char_obj, 1, common_LightGrid_char);

// pybricks._common.LightGrid.on
STATIC mp_obj_t common_LightGrid_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    mp_int_t b = pb_obj_get_pct(brightness);
    for (uint8_t i = 0; i < self->size; i++) {
        for (uint8_t j = 0; j < self->size; j++) {
            set_pixel_brightness(self, i, j, b);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_on_obj, 1, common_LightGrid_on);

// pybricks._common.LightGrid.off
STATIC mp_obj_t common_LightGrid_off(mp_obj_t self_in) {
    common_LightGrid_obj_t *self = MP_OBJ_TO_PTR(self_in);

    for (uint8_t i = 0; i < self->size; i++) {
        for (uint8_t j = 0; j < self->size; j++) {
            set_pixel_brightness(self, i, j, 0);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(common_LightGrid_off_obj, common_LightGrid_off);

// pybricks._common.LightGrid.number
STATIC mp_obj_t common_LightGrid_number(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(number));

    // Currently numbers are only implemented for 5x5 grids
    if (self->size != 5) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    // Get the number
    mp_int_t value = pb_obj_get_int(number);

    // > 99 gets displayed as >
    if (value > 99) {
        light_grid_set_rows_binary(self, pb_font_5x5['>' - 32]);
        return mp_const_none;
    }

    // < -99 gets displayed as <
    if (value < -99) {
        light_grid_set_rows_binary(self, pb_font_5x5['<' - 32]);
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

    // Display the result
    light_grid_set_rows_binary(self, composite);

    // Display one faint dot in the middle to indicate negative
    if (negative) {
        set_pixel_brightness(self, 2, 2, 50);
    }

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

    // Set pixel at the given brightness
    set_pixel_brightness(self, mp_obj_get_int(row), mp_obj_get_int(column), pb_obj_get_pct(brightness));

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
    grid->channels = spike_prime_tlc5955_pwm_channel_map;
    pb_assert(pbdrv_pwm_get_dev(4, &grid->pwm));
    return grid;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_LIGHTGRID
