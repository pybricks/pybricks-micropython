// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_LIGHTGRID

#include <pbio/lightgrid.h>

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"

#include <pybricks/common.h>
#include <pybricks/robotics.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>


// pybricks._common.LightGrid class object
typedef struct _common_LightGrid_obj_t {
    mp_obj_base_t base;
    pbio_lightgrid_t *lightgrid;
    uint8_t *data;
    uint8_t frames;
} common_LightGrid_obj_t;

// Renews memory for a given number of frames
STATIC void common_LightGrid__renew(common_LightGrid_obj_t *self, uint8_t frames) {
    // Grid with/height
    size_t size = pbio_lightgrid_get_size(self->lightgrid);

    // Renew buffer for new number of frames
    self->data = m_renew(uint8_t, self->data, size * size * self->frames, size * size * frames);

    // Save new number of frames
    self->frames = frames;
}

// pybricks._common.LightGrid.char
STATIC mp_obj_t common_LightGrid_char(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(character));

    // Assert that the input is a single character
    GET_STR_DATA_LEN(character, str, len);
    if (len != 1 || str[0] < 32 || str[0] > 126) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Pick corresponding image and display it
    pbio_lightgrid_set_rows(self->lightgrid, pb_font_5x5[str[0] - 32]);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_char_obj, 1, common_LightGrid_char);

static void common_LightGrid_image__extract(mp_obj_t image, size_t size, uint8_t *data) {

    #if MICROPY_PY_BUILTINS_FLOAT
    // If image is a matrix, copy data from there
    if (mp_obj_is_type(image, &pb_type_Matrix_type)) {
        for (size_t r = 0; r < size; r++) {
            for (size_t c = 0; c < size; c++) {
                data[r * size + c] = pb_type_Matrix__get_scalar(image, r, c);
            }
        }
        return;
    }
    #endif // MICROPY_PY_BUILTINS_FLOAT

    // Unpack the main list of rows and get the requested sizes
    mp_obj_t *row_objs, *scalar_objs;
    size_t m;
    mp_obj_get_array(image, &m, &row_objs);
    if (m != size) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Iterate through each of the rows to get the scalars
    for (size_t r = 0; r < size; r++) {
        size_t n;
        mp_obj_get_array(row_objs[r], &n, &scalar_objs);
        if (n != size) {
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
        // Unpack the scalars
        for (size_t c = 0; c < size; c++) {
            data[r * size + c] = pb_obj_get_pct(scalar_objs[c]);
        }
    }
}

// pybricks._common.LightGrid.image
STATIC mp_obj_t common_LightGrid_image(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(image));

    // Allocate and extract image data
    size_t size = pbio_lightgrid_get_size(self->lightgrid);
    common_LightGrid__renew(self, 1);
    common_LightGrid_image__extract(image, size, self->data);

    // Display the image
    pb_assert(pbio_lightgrid_set_image(self->lightgrid, self->data));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_image_obj, 1, common_LightGrid_image);

// pybricks._common.LightGrid.on
STATIC mp_obj_t common_LightGrid_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    uint8_t size = pbio_lightgrid_get_size(self->lightgrid);

    mp_int_t b = pb_obj_get_pct(brightness);
    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            pbio_lightgrid_set_pixel(self->lightgrid, i, j, b);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_on_obj, 1, common_LightGrid_on);

// pybricks._common.LightGrid.off
STATIC mp_obj_t common_LightGrid_off(mp_obj_t self_in) {
    common_LightGrid_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint8_t size = pbio_lightgrid_get_size(self->lightgrid);

    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            pbio_lightgrid_set_pixel(self->lightgrid, i, j, 0);
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


    uint8_t size = pbio_lightgrid_get_size(self->lightgrid);

    // Currently numbers are only implemented for 5x5 grids
    if (size != 5) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    // Get the number
    mp_int_t value = pb_obj_get_int(number);

    // > 99 gets displayed as >
    if (value > 99) {
        pbio_lightgrid_set_rows(self->lightgrid, pb_font_5x5['>' - 32]);
        return mp_const_none;
    }

    // < -99 gets displayed as <
    if (value < -99) {
        pbio_lightgrid_set_rows(self->lightgrid, pb_font_5x5['<' - 32]);
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
    pbio_lightgrid_set_rows(self->lightgrid, composite);

    // Display one faint dot in the middle to indicate negative
    if (negative) {
        pbio_lightgrid_set_pixel(self->lightgrid, 2, 2, 50);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_number_obj, 1, common_LightGrid_number);

// pybricks._common.LightGrid.pattern
STATIC mp_obj_t common_LightGrid_pattern(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(images),
        PB_ARG_REQUIRED(interval));

    // Time between frames
    mp_int_t dt = pb_obj_get_int(interval);

    // Unpack the list of images
    mp_obj_t *image_objs;
    size_t n;
    mp_obj_get_array(images, &n, &image_objs);
    if (n > UINT8_MAX) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Allocate pattern data
    size_t size = pbio_lightgrid_get_size(self->lightgrid);
    common_LightGrid__renew(self, n);

    // Extract pattern data
    for (uint8_t i = 0; i < n; i++) {
        common_LightGrid_image__extract(image_objs[i], size, self->data + size * size * i);
    }

    // Display the image (blocking for now for testing purposes)
    for (uint8_t i = 0; i < n; i++) {
        common_LightGrid_image__extract(image_objs[i], size, self->data + size * size * i);
        pbio_lightgrid_set_image(self->lightgrid, self->data + size * size * i);
        mp_hal_delay_ms(dt);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_pattern_obj, 1, common_LightGrid_pattern);

// pybricks._common.LightGrid.pixel
STATIC mp_obj_t common_LightGrid_pixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(row),
        PB_ARG_REQUIRED(column),
        PB_ARG_DEFAULT_INT(brightness, 100));

    // Set pixel at the given brightness
    pbio_lightgrid_set_pixel(self->lightgrid, mp_obj_get_int(row), mp_obj_get_int(column), pb_obj_get_pct(brightness));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_pixel_obj, 1, common_LightGrid_pixel);

// pybricks._common.LightGrid.text
STATIC mp_obj_t common_LightGrid_text(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightGrid_obj_t, self,
        PB_ARG_REQUIRED(text),
        PB_ARG_DEFAULT_INT(on, 500),
        PB_ARG_DEFAULT_INT(off, 50),
        PB_ARG_DEFAULT_TRUE(wait));

    if (!mp_obj_is_true(wait)) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    // Assert that the input is a single text
    GET_STR_DATA_LEN(text, str, len);

    // Make sure all characters are valid
    for (size_t i = 0; i < len; i++) {
        if (str[0] < 32 || str[0] > 126) {
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
    }

    mp_int_t on_time = pb_obj_get_int(on);
    mp_int_t off_time = pb_obj_get_int(off);

    // Display all characters one by one
    for (size_t i = 0; i < len; i++) {
        pbio_lightgrid_set_rows(self->lightgrid, pb_font_5x5[str[i] - 32]);
        mp_hal_delay_ms(on_time);

        // Some off time so we can see multiple of the same characters
        if (off_time > 0 || i == len - 1) {
            pbio_lightgrid_set_rows(self->lightgrid, pb_font_5x5[' ' - 32]);
            mp_hal_delay_ms(off_time);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(common_LightGrid_text_obj, 1, common_LightGrid_text);

// dir(pybricks.builtins.LightGrid)
STATIC const mp_rom_map_elem_t common_LightGrid_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_char),   MP_ROM_PTR(&common_LightGrid_char_obj)   },
    { MP_ROM_QSTR(MP_QSTR_image),  MP_ROM_PTR(&common_LightGrid_image_obj)  },
    { MP_ROM_QSTR(MP_QSTR_on),     MP_ROM_PTR(&common_LightGrid_on_obj)     },
    { MP_ROM_QSTR(MP_QSTR_off),    MP_ROM_PTR(&common_LightGrid_off_obj)    },
    { MP_ROM_QSTR(MP_QSTR_number), MP_ROM_PTR(&common_LightGrid_number_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel),  MP_ROM_PTR(&common_LightGrid_pixel_obj)  },
    { MP_ROM_QSTR(MP_QSTR_pattern),MP_ROM_PTR(&common_LightGrid_pattern_obj)},
    { MP_ROM_QSTR(MP_QSTR_text),   MP_ROM_PTR(&common_LightGrid_text_obj)   },
};
STATIC MP_DEFINE_CONST_DICT(common_LightGrid_locals_dict, common_LightGrid_locals_dict_table);

// type(pybricks.builtins.LightGrid)
STATIC const mp_obj_type_t pb_type_LightGrid = {
    { &mp_type_type },
    .name = MP_QSTR_LightGrid,
    .locals_dict = (mp_obj_dict_t *)&common_LightGrid_locals_dict,
};

// pybricks._common.LightGrid.__init__
mp_obj_t common_LightGrid_obj_make_new() {
    // Create new light instance
    common_LightGrid_obj_t *self = m_new_obj(common_LightGrid_obj_t);
    // Set type and iodev
    self->base.type = &pb_type_LightGrid;
    pb_assert(pbio_lightgrid_get_dev(&self->lightgrid));
    return self;
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_LIGHTGRID
