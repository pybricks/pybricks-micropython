// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS_IMAGE

#if !MICROPY_ENABLE_FINALISER
#error "MICROPY_ENABLE_FINALISER must be enabled."
#endif

#include <umm_malloc.h>

#include "py/mphal.h"
#include "py/obj.h"

#include <pybricks/parameters.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include <pbio/image.h>
#include <pbio/int_math.h>

#include <pbdrv/display.h>

extern const mp_obj_type_t pb_type_Image;

// pybricks.media.Image class object
typedef struct _pb_type_Image_obj_t {
    mp_obj_base_t base;
    // For images with shared memory, we need to keep a reference to the object
    // that owns the memory.
    mp_obj_t owner;
    pbio_image_t image;
    bool is_display;
} pb_type_Image_obj_t;

static int get_color(mp_obj_t obj) {
    uint8_t max = pbdrv_display_get_max_value();
    if (obj == mp_const_none) {
        return max;
    }
    if (mp_obj_is_int(obj)) {
        return mp_obj_get_int(obj);
    }
    const pbio_color_hsv_t *hsv = pb_type_Color_get_hsv(obj);
    int32_t v = pbio_int_math_bind(hsv->v, 0, 100);
    return max - v * max / 100;
}

mp_obj_t pb_type_Image_display_obj_new(void) {
    pb_type_Image_obj_t *self = mp_obj_malloc(pb_type_Image_obj_t, &pb_type_Image);
    self->owner = MP_OBJ_NULL;
    self->is_display = true;
    self->image = *pbdrv_display_get_image();

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t pb_type_Image_make_new(const mp_obj_type_t *type,
    size_t n_args, size_t n_kw, const mp_obj_t *args) {
    PB_PARSE_ARGS_CLASS(n_args, n_kw, args,
        PB_ARG_REQUIRED(source),
        PB_ARG_DEFAULT_FALSE(sub),
        PB_ARG_DEFAULT_INT(x1, 0),
        PB_ARG_DEFAULT_INT(y1, 0),
        PB_ARG_DEFAULT_NONE(x2),
        PB_ARG_DEFAULT_NONE(y2));

    pb_type_Image_obj_t *self;

    pb_assert_type(source_in, &pb_type_Image);
    pb_type_Image_obj_t *source = MP_OBJ_TO_PTR(source_in);
    if (!mp_obj_is_true(sub_in)) {
        // Copy.
        int width = source->image.width;
        int height = source->image.height;

        void *buf = umm_malloc(width * height * sizeof(uint8_t));
        if (!buf) {
            mp_raise_type(&mp_type_MemoryError);
        }

        self = mp_obj_malloc_with_finaliser(pb_type_Image_obj_t, &pb_type_Image);
        self->owner = MP_OBJ_NULL;
        self->is_display = false;
        pbio_image_init(&self->image, buf, width, height, width);
        pbio_image_draw_image(&self->image, &source->image, 0, 0);
    } else {
        // Sub-image.
        mp_int_t x1 = pb_obj_get_int(x1_in);
        mp_int_t y1 = pb_obj_get_int(y1_in);
        mp_int_t x2 = x2_in == mp_const_none ? source->image.width - 1 : pb_obj_get_int(x2_in);
        mp_int_t y2 = y2_in == mp_const_none ? source->image.height - 1 : pb_obj_get_int(y2_in);
        self = mp_obj_malloc(pb_type_Image_obj_t, &pb_type_Image);
        self->owner = source_in;
        self->is_display = false;
        int width = x2 - x1 + 1;
        int height = y2 - y1 + 1;
        pbio_image_init_sub(&self->image, &source->image, x1, y1, width, height);
    }

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t pb_type_Image_close(mp_obj_t self_in) {
    pb_type_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // If we own the memory, free it.
    if (self->owner == MP_OBJ_NULL && !self->is_display && self->image.pixels) {
        umm_free(self->image.pixels);
        self->image.pixels = NULL;
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Image_close_obj, pb_type_Image_close);

static mp_obj_t pb_type_Image_empty(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_NONE(width),
        PB_ARG_DEFAULT_NONE(height));

    pbio_image_t *display = pbdrv_display_get_image();

    mp_int_t width = width_in == mp_const_none ? display->width : mp_obj_get_int(width_in);
    mp_int_t height = height_in == mp_const_none ? display->height : mp_obj_get_int(height_in);

    if (width < 1 || height < 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("Image width or height is less than 1"));
    }

    void *buf = umm_malloc(width * height * sizeof(uint8_t));
    if (!buf) {
        mp_raise_type(&mp_type_MemoryError);
    }

    pb_type_Image_obj_t *self = mp_obj_malloc_with_finaliser(pb_type_Image_obj_t, &pb_type_Image);
    self->owner = MP_OBJ_NULL;
    self->is_display = false;
    pbio_image_init(&self->image, buf, width, height, width);
    pbio_image_fill(&self->image, 0);

    return MP_OBJ_FROM_PTR(self);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_empty_fun_obj, 0, pb_type_Image_empty);
static MP_DEFINE_CONST_STATICMETHOD_OBJ(pb_type_Image_empty_obj, MP_ROM_PTR(&pb_type_Image_empty_fun_obj));

static void pb_type_Image_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // Read only
    if (dest[0] == MP_OBJ_NULL) {
        pb_type_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);
        if (attr == MP_QSTR_width) {
            dest[0] = mp_obj_new_int(self->image.width);
            return;
        }
        if (attr == MP_QSTR_height) {
            dest[0] = mp_obj_new_int(self->image.height);
            return;
        }
    }
    // Attribute not found, continue lookup in locals dict.
    dest[1] = MP_OBJ_SENTINEL;
}

static mp_obj_t pb_type_Image_clear(mp_obj_t self_in) {
    pb_type_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);

    pbio_image_fill(&self->image, 0);

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_type_Image_clear_obj, pb_type_Image_clear);

static mp_obj_t pb_type_Image_load_image(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(source));

    pb_assert_type(source_in, &pb_type_Image);
    pb_type_Image_obj_t *source = MP_OBJ_TO_PTR(source_in);

    int x = (self->image.width - source->image.width) / 2;
    int y = (self->image.height - source->image.height) / 2;

    pbio_image_fill(&self->image, 0);
    pbio_image_draw_image(&self->image, &source->image, x, y);

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_load_image_obj, 1, pb_type_Image_load_image);

static mp_obj_t pb_type_Image_draw_image(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_REQUIRED(source),
        PB_ARG_DEFAULT_NONE(transparent));

    mp_int_t x = mp_obj_get_int(x_in);
    mp_int_t y = mp_obj_get_int(y_in);
    pb_assert_type(source_in, &pb_type_Image);
    pb_type_Image_obj_t *source = MP_OBJ_TO_PTR(source_in);

    if (transparent_in == mp_const_none) {
        pbio_image_draw_image(&self->image, &source->image, x, y);
    } else {
        int transparent_value = get_color(transparent_in);

        pbio_image_draw_image_transparent(&self->image, &source->image, x, y, transparent_value);
    }

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_draw_image_obj, 1, pb_type_Image_draw_image);

static mp_obj_t pb_type_Image_draw_pixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_DEFAULT_NONE(color));

    mp_int_t x = pb_obj_get_int(x_in);
    mp_int_t y = pb_obj_get_int(y_in);
    int color = get_color(color_in);

    pbio_image_draw_pixel(&self->image, x, y, color);

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_draw_pixel_obj, 1, pb_type_Image_draw_pixel);

static mp_obj_t pb_type_Image_draw_line(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(x1),
        PB_ARG_REQUIRED(y1),
        PB_ARG_REQUIRED(x2),
        PB_ARG_REQUIRED(y2),
        PB_ARG_DEFAULT_INT(width, 1),
        PB_ARG_DEFAULT_NONE(color));

    mp_int_t x1 = pb_obj_get_int(x1_in);
    mp_int_t y1 = pb_obj_get_int(y1_in);
    mp_int_t x2 = pb_obj_get_int(x2_in);
    mp_int_t y2 = pb_obj_get_int(y2_in);
    mp_int_t width = pb_obj_get_int(width_in);
    int color = get_color(color_in);

    pbio_image_draw_thick_line(&self->image, x1, y1, x2, y2, width, color);

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_draw_line_obj, 1, pb_type_Image_draw_line);

static mp_obj_t pb_type_Image_draw_box(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(x1),
        PB_ARG_REQUIRED(y1),
        PB_ARG_REQUIRED(x2),
        PB_ARG_REQUIRED(y2),
        PB_ARG_DEFAULT_INT(r, 0),
        PB_ARG_DEFAULT_FALSE(fill),
        PB_ARG_DEFAULT_NONE(color));

    mp_int_t x1 = pb_obj_get_int(x1_in);
    mp_int_t y1 = pb_obj_get_int(y1_in);
    mp_int_t x2 = pb_obj_get_int(x2_in);
    mp_int_t y2 = pb_obj_get_int(y2_in);
    mp_int_t r = pb_obj_get_int(r_in);
    bool fill = mp_obj_is_true(fill_in);
    int color = get_color(color_in);

    int width = x2 - x1 + 1;
    int height = y2 - y1 + 1;
    if (fill) {
        pbio_image_fill_rounded_rect(&self->image, x1, y1, width, height, r, color);
    } else {
        pbio_image_draw_rounded_rect(&self->image, x1, y1, width, height, r, color);
    }

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_draw_box_obj, 1, pb_type_Image_draw_box);

static mp_obj_t pb_type_Image_draw_circle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_REQUIRED(r),
        PB_ARG_DEFAULT_FALSE(fill),
        PB_ARG_DEFAULT_NONE(color));

    mp_int_t x = pb_obj_get_int(x_in);
    mp_int_t y = pb_obj_get_int(y_in);
    mp_int_t r = pb_obj_get_int(r_in);
    bool fill = mp_obj_is_true(fill_in);
    int color = get_color(color_in);

    if (fill) {
        pbio_image_fill_circle(&self->image, x, y, r, color);
    } else {
        pbio_image_draw_circle(&self->image, x, y, r, color);
    }

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_draw_circle_obj, 1, pb_type_Image_draw_circle);

static mp_obj_t pb_type_Image_draw_text(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_REQUIRED(text),
        PB_ARG_DEFAULT_NONE(text_color),
        PB_ARG_DEFAULT_NONE(background_color));

    mp_int_t x = pb_obj_get_int(x_in);
    mp_int_t y = pb_obj_get_int(y_in);
    size_t text_len;
    const char *text = mp_obj_str_get_data(text_in, &text_len);
    int text_color = get_color(text_color_in);

    const pbio_font_t *font = &pbio_font_terminus_normal_16;

    if (background_color_in != mp_const_none) {
        int background_color = get_color(background_color_in);
        pbio_image_rect_t rect;

        pbio_image_bbox_text(font, text, text_len, &rect);
        pbio_image_fill_rect(&self->image, x + rect.x - 1, y + rect.y - 1 + font->top_max,
            rect.width + 2, rect.height + 2, background_color);
    }

    pbio_image_draw_text(&self->image, font, x, y + font->top_max, text, text_len, text_color);

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_draw_text_obj, 1, pb_type_Image_draw_text);

static mp_obj_t pb_type_Image_print(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // TODO, this is a draft.
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Image_obj_t, self,
        PB_ARG_REQUIRED(text));

    size_t text_len;
    const char *text = mp_obj_str_get_data(text_in, &text_len);

    self->image.print_font = &pbio_font_terminus_normal_16;
    self->image.print_value = 3;

    pbio_image_print(&self->image, text, text_len);

    if (self->is_display) {
        pbdrv_display_update();
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Image_print_obj, 1, pb_type_Image_print);

// dir(pybricks.media.Image)
static const mp_rom_map_elem_t pb_type_Image_locals_dict_table[] = {
    // REVISIT: consider close() method and __enter__/__exit__ for context manager
    // to deterministically free memory if needed.
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_type_Image_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_empty), MP_ROM_PTR(&pb_type_Image_empty_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&pb_type_Image_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_load_image), MP_ROM_PTR(&pb_type_Image_load_image_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_image), MP_ROM_PTR(&pb_type_Image_draw_image_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_pixel), MP_ROM_PTR(&pb_type_Image_draw_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_line), MP_ROM_PTR(&pb_type_Image_draw_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_box), MP_ROM_PTR(&pb_type_Image_draw_box_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&pb_type_Image_draw_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_text), MP_ROM_PTR(&pb_type_Image_draw_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_print), MP_ROM_PTR(&pb_type_Image_print_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_Image_locals_dict, pb_type_Image_locals_dict_table);

// type(pybricks.media.Image)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_Image,
    MP_QSTR_Image,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_Image_make_new,
    attr, pb_type_Image_attr,
    locals_dict, &pb_type_Image_locals_dict);

#endif // PYBRICKS_PY_PARAMETERS_IMAGE
