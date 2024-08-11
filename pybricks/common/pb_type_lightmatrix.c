// SPDX-License-Identifier: MIT
// Copyright (c) 2020-2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_LIGHT_MATRIX

#include <pbio/light_matrix.h>

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"

#include <pybricks/common.h>
#include <pybricks/tools/pb_type_matrix.h>
#include <pybricks/parameters.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_type_enum.h>


// pybricks._common.LightMatrix class object
typedef struct _common_LightMatrix_obj_t {
    mp_obj_base_t base;
    pbio_light_matrix_t *light_matrix;
    uint8_t *data;
    uint8_t frames;
    // Frozen Python implementation of the async text() method.
    mp_obj_t async_text_method;
} common_LightMatrix_obj_t;

// Renews memory for a given number of frames
static void common_LightMatrix__renew(common_LightMatrix_obj_t *self, uint8_t frames) {
    // Matrix with/height
    size_t size = pbio_light_matrix_get_size(self->light_matrix);

    // Renew buffer for new number of frames
    self->data = m_renew(uint8_t, self->data, size * size * self->frames, size * size * frames);

    // Save new number of frames
    self->frames = frames;
}

// pybricks._common.LightMatrix.orientation
static mp_obj_t common_LightMatrix_orientation(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_REQUIRED(up));

    pbio_light_matrix_set_orientation(self->light_matrix, pb_type_enum_get_value(up_in, &pb_enum_type_Side));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_orientation_obj, 1, common_LightMatrix_orientation);

void pb_type_LightMatrix_display_char(pbio_light_matrix_t *light_matrix, mp_obj_t char_in) {
    // Argument must be a qstr or string
    if (!mp_obj_is_qstr(char_in)) {
        pb_assert_type(char_in, &mp_type_str);
    }

    // Assert that the input is a single character
    GET_STR_DATA_LEN(char_in, text, text_len);
    if (text_len != 1 || text[0] < 32 || text[0] > 126) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Pick corresponding icon and display it
    pb_assert(pbio_light_matrix_set_rows(light_matrix, pb_font_5x5[text[0] - 32]));
}

// pybricks._common.LightMatrix.char
static mp_obj_t common_LightMatrix_char(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_REQUIRED(char));

    pb_type_LightMatrix_display_char(self->light_matrix, char_in);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_char_obj, 1, common_LightMatrix_char);

static void common_LightMatrix_icon__extract(mp_obj_t icon_in, size_t size, uint8_t *data) {

    #if MICROPY_PY_BUILTINS_FLOAT
    // If icon is a matrix, copy data from there
    if (mp_obj_is_type(icon_in, &pb_type_Matrix)) {
        for (size_t r = 0; r < size; r++) {
            for (size_t c = 0; c < size; c++) {
                float scalar = pb_type_Matrix_get_scalar(icon_in, r, c);
                scalar = scalar > 100 ? 100 : (scalar < 0 ? 0: scalar);
                data[r * size + c] = (uint8_t)scalar;
            }
        }
        return;
    }
    #endif // MICROPY_PY_BUILTINS_FLOAT

    // Unpack the main list of rows and get the requested sizes
    mp_obj_t *row_objs, *scalar_objs;
    size_t m;
    mp_obj_get_array(icon_in, &m, &row_objs);
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

// pybricks._common.LightMatrix.icon
static mp_obj_t common_LightMatrix_icon(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_REQUIRED(icon));

    // Allocate and extract image data
    size_t size = pbio_light_matrix_get_size(self->light_matrix);
    common_LightMatrix__renew(self, 1);
    common_LightMatrix_icon__extract(icon_in, size, self->data);

    // Display the icon
    pb_assert(pbio_light_matrix_set_image(self->light_matrix, self->data));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_icon_obj, 1, common_LightMatrix_icon);

// pybricks._common.LightMatrix.on
static mp_obj_t common_LightMatrix_on(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_DEFAULT_INT(brightness, 100));

    uint8_t size = pbio_light_matrix_get_size(self->light_matrix);

    mp_int_t brightness = pb_obj_get_pct(brightness_in);

    for (uint8_t i = 0; i < size; i++) {
        for (uint8_t j = 0; j < size; j++) {
            pb_assert(pbio_light_matrix_set_pixel(self->light_matrix, i, j, brightness));
        }
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_on_obj, 1, common_LightMatrix_on);

// pybricks._common.LightMatrix.off
static mp_obj_t common_LightMatrix_off(mp_obj_t self_in) {
    common_LightMatrix_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pb_assert(pbio_light_matrix_clear(self->light_matrix));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(common_LightMatrix_off_obj, common_LightMatrix_off);

void pb_type_LightMatrix_display_number(pbio_light_matrix_t *light_matrix, mp_obj_t number_in) {

    mp_int_t number = pb_obj_get_int(number_in);

    uint8_t size = pbio_light_matrix_get_size(light_matrix);

    // Currently numbers are only implemented for 5x5 matrices
    if (size != 5) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    // > 99 gets displayed as >
    if (number > 99) {
        pb_assert(pbio_light_matrix_set_rows(light_matrix, pb_font_5x5['>' - 32]));
        return;
    }

    // < -99 gets displayed as <
    if (number < -99) {
        pb_assert(pbio_light_matrix_set_rows(light_matrix, pb_font_5x5['<' - 32]));
        return;
    }

    // Remember sign but make value positive
    bool negative = number < 0;
    if (negative) {
        number = -number;
    }

    // Compose number as two digits
    uint8_t composite[5];
    for (uint8_t i = 0; i < 5; i++) {
        composite[i] = pb_digits_5x2[number / 10][i] << 3 | pb_digits_5x2[number % 10][i];
    }

    // Display the result
    pb_assert(pbio_light_matrix_set_rows(light_matrix, composite));

    // Display one faint dot in the middle to indicate negative
    if (negative) {
        pb_assert(pbio_light_matrix_set_pixel(light_matrix, 2, 2, 50));
    }
}

// pybricks._common.LightMatrix.number
static mp_obj_t common_LightMatrix_number(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_REQUIRED(number));

    // Display the number
    pb_type_LightMatrix_display_number(self->light_matrix, number_in);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_number_obj, 1, common_LightMatrix_number);

// pybricks._common.LightMatrix.animate
static mp_obj_t common_LightMatrix_animate(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_REQUIRED(icons),
        PB_ARG_REQUIRED(interval));

    // Time between frames
    mp_int_t interval = pb_obj_get_int(interval_in);

    // Unpack the list of icons
    mp_obj_t *icon_objs;
    size_t n;
    mp_obj_get_array(icons_in, &n, &icon_objs);
    if (n > UINT8_MAX || n < 2) {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // Allocate animation data
    size_t size = pbio_light_matrix_get_size(self->light_matrix);
    common_LightMatrix__renew(self, n);

    // Extract animation data
    for (uint8_t i = 0; i < n; i++) {
        common_LightMatrix_icon__extract(icon_objs[i], size, self->data + size * size * i);
    }

    // Activate the animation
    pbio_light_matrix_start_animation(self->light_matrix, self->data, self->frames, interval);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_animate_obj, 1, common_LightMatrix_animate);

// pybricks._common.LightMatrix.pixel
static mp_obj_t common_LightMatrix_pixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_REQUIRED(row),
        PB_ARG_REQUIRED(column),
        PB_ARG_DEFAULT_INT(brightness, 100));

    // Set pixel at the given brightness
    pb_assert(pbio_light_matrix_set_pixel(self->light_matrix, pb_obj_get_int(row_in), pb_obj_get_int(column_in), pb_obj_get_pct(brightness_in)));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_pixel_obj, 1, common_LightMatrix_pixel);

// pybricks._common.LightMatrix.text
static mp_obj_t common_LightMatrix_text(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        common_LightMatrix_obj_t, self,
        PB_ARG_REQUIRED(text),
        PB_ARG_DEFAULT_INT(on, 500),
        PB_ARG_DEFAULT_INT(off, 50));

    if (pb_module_tools_run_loop_is_active()) {
        if (self->async_text_method == MP_OBJ_NULL) {
            self->async_text_method = pb_function_import_helper(MP_QSTR__hub_extra, MP_QSTR_light_matrix_text_async);
        }
        mp_obj_t args[] = {
            MP_OBJ_FROM_PTR(self),
            text_in,
            on_in,
            off_in,
        };
        return mp_call_function_n_kw(self->async_text_method, MP_ARRAY_SIZE(args), 0, args);
    }

    // Assert that the input is a single text
    GET_STR_DATA_LEN(text_in, text, text_len);

    // Make sure all characters are valid
    for (size_t i = 0; i < text_len; i++) {
        if (text[0] < 32 || text[0] > 126) {
            pb_assert(PBIO_ERROR_INVALID_ARG);
        }
    }

    mp_int_t on = pb_obj_get_int(on_in);
    mp_int_t off = pb_obj_get_int(off_in);

    // Display all characters one by one
    for (size_t i = 0; i < text_len; i++) {
        pb_assert(pbio_light_matrix_set_rows(self->light_matrix, pb_font_5x5[text[i] - 32]));
        mp_hal_delay_ms(on);

        // Some off time so we can see multiple of the same characters
        if (off > 0 || i == text_len - 1) {
            pb_assert(pbio_light_matrix_clear(self->light_matrix));
            mp_hal_delay_ms(off);
        }
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(common_LightMatrix_text_obj, 1, common_LightMatrix_text);

// dir(pybricks.builtins.LightMatrix)
static const mp_rom_map_elem_t common_LightMatrix_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_char),            MP_ROM_PTR(&common_LightMatrix_char_obj)            },
    { MP_ROM_QSTR(MP_QSTR_icon),            MP_ROM_PTR(&common_LightMatrix_icon_obj)            },
    { MP_ROM_QSTR(MP_QSTR_number),          MP_ROM_PTR(&common_LightMatrix_number_obj)          },
    { MP_ROM_QSTR(MP_QSTR_off),             MP_ROM_PTR(&common_LightMatrix_off_obj)             },
    { MP_ROM_QSTR(MP_QSTR_on),              MP_ROM_PTR(&common_LightMatrix_on_obj)              },
    { MP_ROM_QSTR(MP_QSTR_animate),         MP_ROM_PTR(&common_LightMatrix_animate_obj)         },
    { MP_ROM_QSTR(MP_QSTR_pixel),           MP_ROM_PTR(&common_LightMatrix_pixel_obj)           },
    { MP_ROM_QSTR(MP_QSTR_orientation),     MP_ROM_PTR(&common_LightMatrix_orientation_obj)     },
    { MP_ROM_QSTR(MP_QSTR_text),            MP_ROM_PTR(&common_LightMatrix_text_obj)            },
};
static MP_DEFINE_CONST_DICT(common_LightMatrix_locals_dict, common_LightMatrix_locals_dict_table);

// type(pybricks.builtins.LightMatrix)
static MP_DEFINE_CONST_OBJ_TYPE(pb_type_LightMatrix,
    MP_QSTR_LightMatrix,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_LightMatrix_locals_dict);

// pybricks._common.LightMatrix.__init__
mp_obj_t pb_type_LightMatrix_obj_new(pbio_light_matrix_t *light_matrix) {
    common_LightMatrix_obj_t *self = mp_obj_malloc(common_LightMatrix_obj_t, &pb_type_LightMatrix);
    self->light_matrix = light_matrix;
    pbio_light_matrix_set_orientation(light_matrix, PBIO_GEOMETRY_SIDE_TOP);
    self->async_text_method = MP_OBJ_NULL;
    return MP_OBJ_FROM_PTR(self);
}

#endif // PYBRICKS_PY_COMMON && PYBRICKS_PY_COMMON_LIGHT_MATRIX
