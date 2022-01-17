// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2019-2021 The Pybricks Authors

// class Image
//
// Image manipulation on ev3dev using the GRX3 graphics library. This can be
// used for both in-memory images and writing directly to the screen.

#include <string.h>

#include <grx-3.0.h>

#include <pbio/color.h>

#include "py/mpconfig.h"
#include "py/misc.h"
#include "py/mpprint.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

#include "pb_ev3dev_types.h"

#include <pybricks/parameters.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

typedef struct _ev3dev_Image_obj_t {
    mp_obj_base_t base;
    mp_obj_t width;
    mp_obj_t height;
    mp_obj_t buffer; // only used by _screen_
    gboolean cleared; // only used by _screen_
    GrxContext *context;
    void *mem; // don't touch - needed for GC pressure
    GrxTextOptions *text_options;
    gint print_x;
    gint print_y;
} ev3dev_Image_obj_t;

// map Pybricks color type to GRX color value.
STATIC GrxColor map_color(mp_obj_t obj) {
    if (obj == mp_const_none) {
        return GRX_COLOR_NONE;
    }

    pbio_color_rgb_t rgb;
    pbio_color_hsv_to_rgb(pb_type_Color_get_hsv(obj), &rgb);
    return grx_color_get(rgb.r, rgb.g, rgb.b);
}

STATIC mp_obj_t ev3dev_Image_new(GrxContext *context) {
    ev3dev_Image_obj_t *self = m_new_obj_with_finaliser(ev3dev_Image_obj_t);

    self->base.type = &pb_type_ev3dev_Image.type;
    self->context = context;
    self->mem = context->frame.base_address.plane0;
    self->width = mp_obj_new_int(grx_context_get_width(self->context));
    self->height = mp_obj_new_int(grx_context_get_height(self->context));

    pb_type_ev3dev_Font_init();
    GrxFont *font = pb_ev3dev_Font_obj_get_font(pb_const_ev3dev_font_DEFAULT);
    self->text_options = grx_text_options_new(font, GRX_COLOR_BLACK);

    // only the screen needs to be cleared on first use
    self->cleared = context != grx_get_screen_context();

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t ev3dev_Image_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    enum { ARG_source, ARG_sub, ARG_x1, ARG_y1, ARG_x2, ARG_y2 };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_source, MP_ARG_REQUIRED | MP_ARG_OBJ, { } },
        { MP_QSTR_sub, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = FALSE} },
        { MP_QSTR_x1, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_y1, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_x2, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_y2, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, arg_vals);

    GrxContext *context = NULL;

    mp_obj_t source_in = arg_vals[ARG_source].u_obj;
    if (mp_obj_is_qstr(source_in) && MP_OBJ_QSTR_VALUE(source_in) == MP_QSTR__screen_) {
        // special case '_screen_' creates image that draws directly to screen
        context = grx_context_ref(grx_get_screen_context());
    } else if (mp_obj_is_str(source_in)) {
        const char *filename = mp_obj_str_get_str(source_in);

        // add file extension if missing
        char *filename_ext = NULL;
        if (!g_str_has_suffix(filename, ".png") && !g_str_has_suffix(filename, ".PNG")) {
            filename_ext = g_strconcat(filename, ".png", NULL);
            filename = filename_ext;
        }

        gint w, h;
        if (!grx_query_png_file(filename, &w, &h)) {
            mp_obj_t ex = mp_obj_new_exception_msg_varg(&mp_type_OSError,
                MP_ERROR_TEXT("'%s' is not a .png file"), filename);
            g_free(filename_ext);
            nlr_raise(ex);
        }

        GrxFrameMemory mem;
        mem.plane0 = m_malloc(grx_screen_get_context_size(w, h));
        context = grx_context_new(w, h, &mem, NULL);
        if (!context) {
            g_free(filename_ext);
            mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("failed to allocate context for image"));
        }

        GError *error = NULL;
        if (!grx_context_load_from_png(context, filename, FALSE, &error)) {
            mp_obj_t ex = mp_obj_new_exception_msg_varg(&mp_type_OSError,
                MP_ERROR_TEXT("Failed to load '%s': %s"), filename, error->message);
            g_free(filename_ext);
            g_error_free(error);
            nlr_raise(ex);
        }

        g_free(filename_ext);
    } else if (mp_obj_is_type(source_in, &pb_type_ev3dev_Image.type)) {
        ev3dev_Image_obj_t *image = MP_OBJ_TO_PTR(source_in);
        if (arg_vals[ARG_sub].u_bool) {
            mp_int_t x1 = pb_obj_get_int(arg_vals[ARG_x1].u_obj);
            mp_int_t y1 = pb_obj_get_int(arg_vals[ARG_y1].u_obj);
            mp_int_t x2 = pb_obj_get_int(arg_vals[ARG_x2].u_obj);
            mp_int_t y2 = pb_obj_get_int(arg_vals[ARG_y2].u_obj);
            context = grx_context_new_subcontext(x1, y1, x2, y2, image->context, NULL);
        } else {
            gint w = grx_context_get_width(image->context);
            gint h = grx_context_get_height(image->context);
            GrxFrameMemory mem;
            mem.plane0 = m_malloc(grx_screen_get_context_size(w, h));
            context = grx_context_new(w, h, &mem, NULL);
            grx_context_bit_blt(context, 0, 0, image->context, 0, 0,
                w - 1, h - 1, GRX_COLOR_MODE_WRITE);
        }
    }

    if (!context) {
        mp_raise_TypeError(MP_ERROR_TEXT("Argument must be str or Image"));
    }

    return ev3dev_Image_new(context);
}

STATIC mp_obj_t ev3dev_Image_empty(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_width, ARG_height };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_width, MP_ARG_OBJ, { .u_obj = mp_obj_new_int(grx_get_screen_width())} },
        { MP_QSTR_height, MP_ARG_OBJ, { .u_obj = mp_obj_new_int(grx_get_screen_height())} },
    };

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, arg_vals);

    mp_int_t width = pb_obj_get_int(arg_vals[ARG_width].u_obj);
    mp_int_t height = pb_obj_get_int(arg_vals[ARG_height].u_obj);
    if (width <= 0 || height <= 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("width and height must be greater than 0"));
    }
    GrxFrameMemory mem;
    mem.plane0 = m_malloc(grx_screen_get_context_size(width, height));
    GrxContext *context = grx_context_new(width, height, &mem, NULL);
    if (!context) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to create graphics context"));
    }
    grx_context_clear(context, GRX_COLOR_WHITE);
    return ev3dev_Image_new(context);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_empty_fun_obj, 0, ev3dev_Image_empty);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(ev3dev_Image_empty_obj, MP_ROM_PTR(&ev3dev_Image_empty_fun_obj));

STATIC mp_obj_t ev3dev_Image___del__(mp_obj_t self_in) {
    ev3dev_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);
    grx_text_options_unref(self->text_options);
    grx_context_unref(self->context);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3dev_Image___del___obj, ev3dev_Image___del__);

// Ensure that screen has been cleared before we start drawing anything else
STATIC void clear_once(ev3dev_Image_obj_t *self) {
    if (self->cleared) {
        return;
    }
    grx_context_clear(self->context, GRX_COLOR_WHITE);
    self->cleared = TRUE;
}

STATIC mp_obj_t ev3dev_Image_clear(mp_obj_t self_in) {
    ev3dev_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);
    clear_once(self);
    grx_context_clear(self->context, GRX_COLOR_WHITE);
    self->print_x = 0;
    self->print_y = 0;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3dev_Image_clear_obj, ev3dev_Image_clear);

STATIC mp_obj_t ev3dev_Image_draw_pixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_DEFAULT_OBJ(color, pb_Color_BLACK_obj));

    mp_int_t x = pb_obj_get_int(x_in);
    mp_int_t y = pb_obj_get_int(y_in);
    GrxColor color = map_color(color_in);

    clear_once(self);
    grx_set_current_context(self->context);
    grx_draw_pixel(x, y, color);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_draw_pixel_obj, 1, ev3dev_Image_draw_pixel);

STATIC mp_obj_t ev3dev_Image_draw_line(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Image_obj_t, self,
        PB_ARG_REQUIRED(x1),
        PB_ARG_REQUIRED(y1),
        PB_ARG_REQUIRED(x2),
        PB_ARG_REQUIRED(y2),
        PB_ARG_DEFAULT_INT(width, 1),
        PB_ARG_DEFAULT_OBJ(color, pb_Color_BLACK_obj));

    mp_int_t x1 = pb_obj_get_int(x1_in);
    mp_int_t y1 = pb_obj_get_int(y1_in);
    mp_int_t x2 = pb_obj_get_int(x2_in);
    mp_int_t y2 = pb_obj_get_int(y2_in);
    mp_int_t width = pb_obj_get_int(width_in);
    GrxColor color = map_color(color_in);

    clear_once(self);
    grx_set_current_context(self->context);
    if (width == 1) {
        grx_draw_line(x1, y1, x2, y2, color);
    } else {
        GrxLineOptions options = { .color = color, .width = width };
        grx_draw_line_with_options(x1, y1, x2, y2, &options);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_draw_line_obj, 1, ev3dev_Image_draw_line);

STATIC mp_obj_t ev3dev_Image_draw_box(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Image_obj_t, self,
        PB_ARG_REQUIRED(x1),
        PB_ARG_REQUIRED(y1),
        PB_ARG_REQUIRED(x2),
        PB_ARG_REQUIRED(y2),
        PB_ARG_DEFAULT_INT(r, 0),
        PB_ARG_DEFAULT_FALSE(fill),
        PB_ARG_DEFAULT_OBJ(color, pb_Color_BLACK_obj));

    mp_int_t x1 = pb_obj_get_int(x1_in);
    mp_int_t y1 = pb_obj_get_int(y1_in);
    mp_int_t x2 = pb_obj_get_int(x2_in);
    mp_int_t y2 = pb_obj_get_int(y2_in);
    mp_int_t r = pb_obj_get_int(r_in);
    GrxColor color = map_color(color_in);

    clear_once(self);
    grx_set_current_context(self->context);
    if (mp_obj_is_true(fill_in)) {
        if (r > 0) {
            grx_draw_filled_rounded_box(x1, y1, x2, y2, r, color);
        } else {
            grx_draw_filled_box(x1, y1, x2, y2, color);
        }
    } else {
        if (r > 0) {
            grx_draw_rounded_box(x1, y1, x2, y2, r, color);
        } else {
            grx_draw_box(x1, y1, x2, y2, color);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_draw_box_obj, 1, ev3dev_Image_draw_box);

STATIC mp_obj_t ev3dev_Image_draw_circle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_REQUIRED(r),
        PB_ARG_DEFAULT_FALSE(fill),
        PB_ARG_DEFAULT_OBJ(color, pb_Color_BLACK_obj));

    mp_int_t x = pb_obj_get_int(x_in);
    mp_int_t y = pb_obj_get_int(y_in);
    mp_int_t r = pb_obj_get_int(r_in);
    bool fill = mp_obj_is_true(fill_in);
    GrxColor color = map_color(color_in);

    clear_once(self);
    grx_set_current_context(self->context);
    if (fill) {
        grx_draw_filled_circle(x, y, r, color);
    } else {
        grx_draw_circle(x, y, r, color);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_draw_circle_obj, 1, ev3dev_Image_draw_circle);

STATIC mp_obj_t ev3dev_Image_draw_image(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_REQUIRED(source),
        PB_ARG_DEFAULT_NONE(transparent));

    mp_int_t x = pb_obj_get_int(x_in);
    mp_int_t y = pb_obj_get_int(y_in);
    if (mp_obj_is_str(source_in)) {
        mp_obj_t args[1] = { source_in };
        source_in = ev3dev_Image_make_new(&pb_type_ev3dev_Image.type, 1, 0, args);
    }
    if (!mp_obj_is_type(source_in, &pb_type_ev3dev_Image.type)) {
        mp_raise_TypeError(MP_ERROR_TEXT("Image object is required"));
    }
    ev3dev_Image_obj_t *source = MP_OBJ_TO_PTR(source_in);
    GrxColor transparent = map_color(transparent_in);

    clear_once(self);
    grx_context_bit_blt(self->context, x, y, source->context, 0, 0,
        grx_context_get_max_x(source->context), grx_context_get_max_y(source->context),
        transparent == GRX_COLOR_NONE ? GRX_COLOR_MODE_WRITE : grx_color_to_image_mode(transparent));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_draw_image_obj, 1, ev3dev_Image_draw_image);

STATIC mp_obj_t ev3dev_Image_load_image(mp_obj_t self_in, mp_obj_t source_in) {
    ev3dev_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (mp_obj_is_str(source_in)) {
        mp_obj_t args[1] = { source_in };
        source_in = ev3dev_Image_make_new(&pb_type_ev3dev_Image.type, 1, 0, args);
    }

    if (!mp_obj_is_type(source_in, &pb_type_ev3dev_Image.type)) {
        mp_raise_TypeError(MP_ERROR_TEXT("source must be Image or str"));
    }

    ev3dev_Image_obj_t *source = MP_OBJ_TO_PTR(source_in);

    mp_obj_t x = mp_obj_new_int((mp_obj_get_int(self->width) - mp_obj_get_int(source->width)) / 2);
    mp_obj_t y = mp_obj_new_int((mp_obj_get_int(self->height) - mp_obj_get_int(source->height)) / 2);

    // if the destination is the screen, then we double-buffer to prevent flicker
    if (self->context == grx_get_screen_context()) {
        if (self->buffer == MP_OBJ_NULL) {
            mp_obj_t args[2] = { self->width, self->height };
            mp_map_t kw_args;
            mp_map_init(&kw_args, 0);
            self->buffer = ev3dev_Image_empty(MP_ARRAY_SIZE(args), args, &kw_args);
        }

        ev3dev_Image_clear(self->buffer);

        mp_obj_t args[4] = { self->buffer, x, y, source_in };
        mp_map_t kw_args;
        mp_map_init(&kw_args, 0);
        ev3dev_Image_draw_image(MP_ARRAY_SIZE(args), args, &kw_args);

        source_in = self->buffer;
        x = MP_OBJ_NEW_SMALL_INT(0);
        y = MP_OBJ_NEW_SMALL_INT(0);
    } else {
        ev3dev_Image_clear(self_in);
    }

    mp_obj_t args[4] = { self_in, x, y, source_in };
    mp_map_t kw_args;
    mp_map_init(&kw_args, 0);
    ev3dev_Image_draw_image(MP_ARRAY_SIZE(args), args, &kw_args);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ev3dev_Image_load_image_obj, ev3dev_Image_load_image);

STATIC mp_obj_t ev3dev_Image_draw_text(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Image_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_REQUIRED(text),
        PB_ARG_DEFAULT_OBJ(text_color, pb_Color_BLACK_obj),
        PB_ARG_DEFAULT_NONE(background_color));

    mp_int_t x = pb_obj_get_int(x_in);
    mp_int_t y = pb_obj_get_int(y_in);
    if (!mp_obj_is_str_or_bytes(text_in)) {
        vstr_t vstr;
        mp_print_t print;
        vstr_init_print(&vstr, 16, &print);
        mp_obj_print_helper(&print, text_in, PRINT_STR);
        text_in = mp_obj_new_str_from_vstr(&mp_type_str, &vstr);
    }
    const char *text = mp_obj_str_get_str(text_in);
    GrxColor text_color = map_color(text_color_in);
    GrxColor background_color = map_color(background_color_in);

    clear_once(self);
    grx_set_current_context(self->context);
    grx_text_options_set_fg_color(self->text_options, text_color);
    grx_text_options_set_bg_color(self->text_options, background_color);
    if (background_color != GRX_COLOR_NONE) {
        GrxFont *font = grx_text_options_get_font(self->text_options);
        gint w = grx_font_get_text_width(font, text);
        gint h = grx_font_get_text_height(font, text);
        grx_draw_filled_box(x, y, x + w - 1, y + h - 1, background_color);
    }
    grx_draw_text(text, x, y, self->text_options);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_draw_text_obj, 1, ev3dev_Image_draw_text);

STATIC mp_obj_t ev3dev_Image_set_font(mp_obj_t self_in, mp_obj_t font_in) {
    ev3dev_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GrxFont *font = pb_ev3dev_Font_obj_get_font(font_in);

    grx_text_options_set_font(self->text_options, font);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ev3dev_Image_set_font_obj, ev3dev_Image_set_font);

// copy of mp_builtin_print modified to print to vstr
STATIC mp_obj_t ev3dev_Image_print(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_sep, ARG_end };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_sep, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR__space_)} },
        { MP_QSTR_end, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR__0x0a_)} },
    };

    // parse args (a union is used to reduce the amount of C stack that is needed)
    union {
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        size_t len[2];
    } u;
    mp_arg_parse_all(0, NULL, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, u.args);

    ev3dev_Image_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    // extract the objects first because we are going to use the other part of the union
    mp_obj_t sep = u.args[ARG_sep].u_obj;
    mp_obj_t end = u.args[ARG_end].u_obj;
    const char *sep_data = mp_obj_str_get_data(sep, &u.len[0]);
    const char *end_data = mp_obj_str_get_data(end, &u.len[1]);

    vstr_t vstr;
    mp_print_t print;
    vstr_init_print(&vstr, 16, &print);

    for (size_t i = 1; i < n_args; i++) {
        if (i > 1) {
            mp_print_strn(&print, sep_data, u.len[0], 0, 0, 0);
        }
        mp_obj_print_helper(&print, pos_args[i], PRINT_STR);
    }
    mp_print_strn(&print, end_data, u.len[1], 0, 0, 0);

    clear_once(self);
    grx_set_current_context(self->context);
    grx_text_options_set_fg_color(self->text_options, GRX_COLOR_BLACK);
    grx_text_options_set_bg_color(self->text_options, GRX_COLOR_WHITE);
    GrxFont *font = grx_text_options_get_font(self->text_options);
    gint font_height = grx_font_get_height(font);
    gchar **lines = g_strsplit(vstr_null_terminated_str(&vstr), "\n", -1);
    for (gchar **l = lines; *l; l++) {
        // if this is not the first line, then we had a newline and need to advance
        if (l != lines) {
            self->print_x = 0;
            self->print_y += font_height;
        }
        // if printing would run off of the bottom of the screen, scroll
        // everything on the screen up enough to fit one more line
        gint screen_height = grx_get_height();
        gint over = self->print_y + font_height - screen_height;
        if (over > 0) {
            gint max_x = grx_get_max_x();
            for (int y = 0; y < screen_height; y++) {
                const GrxColor *scan_line = grx_get_scanline(0, max_x, y + over, NULL);
                if (scan_line) {
                    grx_put_scanline(0, max_x, y, scan_line, GRX_COLOR_MODE_WRITE);
                } else {
                    grx_draw_hline(0, max_x, y, GRX_COLOR_WHITE);
                }
            }
            self->print_y -= over;
        }
        gint w = grx_font_get_text_width(font, *l);
        gint h = grx_font_get_text_height(font, *l);
        grx_draw_filled_box(self->print_x, self->print_y,
            self->print_x + w - 1, self->print_y + h - 1, GRX_COLOR_WHITE);
        grx_draw_text(*l, self->print_x, self->print_y, self->text_options);
        self->print_x += w;
    }
    g_strfreev(lines);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Image_print_obj, 1, ev3dev_Image_print);

STATIC mp_obj_t ev3dev_Image_save(mp_obj_t self_in, mp_obj_t filename_in) {
    ev3dev_Image_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const char *filename = mp_obj_str_get_str(filename_in);

    // add file extension if missing
    char *filename_ext = NULL;
    if (!g_str_has_suffix(filename, ".png") && !g_str_has_suffix(filename, ".PNG")) {
        filename_ext = g_strconcat(filename, ".png", NULL);
        filename = filename_ext;
    }

    GError *error = NULL;
    gboolean ok = grx_context_save_to_png(self->context, filename, &error);
    g_free(filename_ext);
    if (!ok) {
        mp_obj_t ex = mp_obj_new_exception_msg_varg(&mp_type_OSError,
            MP_ERROR_TEXT("Failed to save image: %s"), error->message);
        g_error_free(error);
        nlr_raise(ex);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(ev3dev_Image_save_obj, ev3dev_Image_save);

STATIC const mp_rom_map_elem_t ev3dev_Image_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_empty),       MP_ROM_PTR(&ev3dev_Image_empty_obj)                    },
    { MP_ROM_QSTR(MP_QSTR___del__),     MP_ROM_PTR(&ev3dev_Image___del___obj)                  },
    { MP_ROM_QSTR(MP_QSTR_clear),       MP_ROM_PTR(&ev3dev_Image_clear_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_draw_pixel),  MP_ROM_PTR(&ev3dev_Image_draw_pixel_obj)               },
    { MP_ROM_QSTR(MP_QSTR_draw_line),   MP_ROM_PTR(&ev3dev_Image_draw_line_obj)                },
    { MP_ROM_QSTR(MP_QSTR_draw_box),    MP_ROM_PTR(&ev3dev_Image_draw_box_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&ev3dev_Image_draw_circle_obj)              },
    { MP_ROM_QSTR(MP_QSTR_draw_image),  MP_ROM_PTR(&ev3dev_Image_draw_image_obj)               },
    { MP_ROM_QSTR(MP_QSTR_load_image),  MP_ROM_PTR(&ev3dev_Image_load_image_obj)               },
    { MP_ROM_QSTR(MP_QSTR_draw_text),   MP_ROM_PTR(&ev3dev_Image_draw_text_obj)                },
    { MP_ROM_QSTR(MP_QSTR_set_font),    MP_ROM_PTR(&ev3dev_Image_set_font_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_print),       MP_ROM_PTR(&ev3dev_Image_print_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_save),        MP_ROM_PTR(&ev3dev_Image_save_obj)                     },
};
STATIC MP_DEFINE_CONST_DICT(ev3dev_Image_locals_dict, ev3dev_Image_locals_dict_table);

STATIC const pb_attr_dict_entry_t ev3dev_Image_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_width, ev3dev_Image_obj_t, width),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_height, ev3dev_Image_obj_t, height),
};

const pb_obj_with_attr_type_t pb_type_ev3dev_Image = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = MP_QSTR_Image,
        .make_new = ev3dev_Image_make_new,
        .attr = pb_attribute_handler,
        .locals_dict = (mp_obj_dict_t *)&ev3dev_Image_locals_dict,
    },
    .attr_dict = ev3dev_Image_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(ev3dev_Image_attr_dict),
};
