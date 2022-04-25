// SPDX-License-Identifier: MIT
// Copyright (c) 2013, 2014 Damien P. George
// Copyright (c) 2019-2021 The Pybricks Authors

// class Screen

#include <string.h>

#include <grx-3.0.h>

#include "py/mpconfig.h"
#include "py/misc.h"
#include "py/mpprint.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "pb_ev3dev_types.h"

#include <pybricks/util_mp/pb_obj_helper.h>

typedef struct _ev3dev_Font_obj_t {
    mp_obj_base_t base;
    GrxFont *font;
    mp_obj_t family;
    mp_obj_t style;
    mp_obj_t width;
    mp_obj_t height;
} ev3dev_Font_obj_t;

ev3dev_Font_obj_t pb_const_ev3dev_Font_DEFAULT_obj;

STATIC void ev3dev_Font_init(ev3dev_Font_obj_t *self, GrxFont *font) {
    self->base.type = &pb_type_ev3dev_Font.type;
    self->font = font;

    const char *family = grx_font_get_family(font);
    self->family = mp_obj_new_str(family, strlen(family));
    const char *style = grx_font_get_style(font);
    self->style = mp_obj_new_str(style, strlen(style));
    self->width = mp_obj_new_int(grx_font_get_width(font));
    self->height = mp_obj_new_int(grx_font_get_height(font));
}

// This must be called from module.__init__() of module that includes this type
// otherwise we will crash when trying to access attributes!
void pb_type_ev3dev_Font_init(void) {
    if (pb_const_ev3dev_Font_DEFAULT_obj.font) {
        // already initialized
        return;
    }

    GError *error = NULL;
    GrxFont *font = grx_font_load("Lucida", 12, &error);
    if (!font) {
        mp_obj_t ex = mp_obj_new_exception_msg_varg(
            &mp_type_RuntimeError, "Failed to load default font: %s", error->message);
        g_error_free(error);
        nlr_raise(ex);
    }
    ev3dev_Font_init(&pb_const_ev3dev_Font_DEFAULT_obj, font);
}

STATIC mp_obj_t ev3dev_Font_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    enum { ARG_family, ARG_size, ARG_bold, ARG_monospace, ARG_lang, ARG_script };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_family, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_size, MP_ARG_INT, {.u_int = 12} },
        { MP_QSTR_bold, MP_ARG_BOOL, {.u_bool = FALSE} },
        { MP_QSTR_monospace, MP_ARG_BOOL, {.u_bool = FALSE} },
        { MP_QSTR_lang, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_script, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, arg_vals);

    const char *family = NULL;
    if (arg_vals[ARG_family].u_obj != mp_const_none) {
        family = mp_obj_str_get_str(arg_vals[ARG_family].u_obj);
    }

    mp_int_t size = arg_vals[ARG_size].u_int;
    mp_int_t dpi = -1; // use screen dpi
    GrxFontWeight weight = arg_vals[ARG_bold].u_bool ? GRX_FONT_WEIGHT_BOLD : GRX_FONT_WEIGHT_REGULAR;
    GrxFontSlant slant = GRX_FONT_SLANT_REGULAR;
    GrxFontWidth width = GRX_FONT_WIDTH_REGULAR;
    bool monospace = arg_vals[ARG_monospace].u_bool;

    const char *lang = NULL;
    if (arg_vals[ARG_lang].u_obj != mp_const_none) {
        lang = mp_obj_str_get_str(arg_vals[ARG_lang].u_obj);
    }

    const char *script = NULL;
    if (arg_vals[ARG_script].u_obj != mp_const_none) {
        script = mp_obj_str_get_str(arg_vals[ARG_script].u_obj);
        if (strlen(script) != 4) {
            mp_raise_ValueError(MP_ERROR_TEXT("script code must have 4 characters"));
        }
    }

    GError *error = NULL;
    GrxFont *font = grx_font_load_full(
        family, size, dpi, weight, slant, width, monospace, lang, script, &error);
    if (!font) {
        mp_obj_t ex = mp_obj_new_exception_msg_varg(
            &mp_type_RuntimeError, "Failed to load font: %s", error->message);
        g_error_free(error);
        nlr_raise(ex);
    }

    ev3dev_Font_obj_t *self = m_new_obj_with_finaliser(ev3dev_Font_obj_t);
    ev3dev_Font_init(self, font);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t ev3dev_Font___del__(mp_obj_t self_in) {
    ev3dev_Font_obj_t *self = MP_OBJ_TO_PTR(self_in);
    grx_font_unref(self->font);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3dev_Font___del___obj, ev3dev_Font___del__);

STATIC mp_obj_t ev3dev_Font_text_width(mp_obj_t self_in, mp_obj_t text_in) {
    ev3dev_Font_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(grx_font_get_text_width(self->font, mp_obj_str_get_str(text_in)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ev3dev_Font_text_width_obj, ev3dev_Font_text_width);

STATIC mp_obj_t ev3dev_Font_text_height(mp_obj_t self_in, mp_obj_t text_in) {
    ev3dev_Font_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(grx_font_get_text_height(self->font, mp_obj_str_get_str(text_in)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(ev3dev_Font_text_height_obj, ev3dev_Font_text_height);

STATIC const mp_rom_map_elem_t ev3dev_Font_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_DEFAULT), MP_ROM_PTR(&pb_const_ev3dev_Font_DEFAULT_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&ev3dev_Font___del___obj) },
    { MP_ROM_QSTR(MP_QSTR_text_width), MP_ROM_PTR(&ev3dev_Font_text_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_text_height), MP_ROM_PTR(&ev3dev_Font_text_height_obj) },
};
STATIC MP_DEFINE_CONST_DICT(ev3dev_Font_locals_dict, ev3dev_Font_locals_dict_table);

STATIC const pb_attr_dict_entry_t ev3dev_Font_attr_dict[] = {
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_family, ev3dev_Font_obj_t, family),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_style, ev3dev_Font_obj_t, style),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_width, ev3dev_Font_obj_t, width),
    PB_DEFINE_CONST_ATTR_RO(MP_QSTR_height, ev3dev_Font_obj_t, height),
};

const pb_obj_with_attr_type_t pb_type_ev3dev_Font = {
    .type = {
        .base = { .type = &mp_type_type },
        .name = MP_QSTR_Font,
        .make_new = ev3dev_Font_make_new,
        .attr = pb_attribute_handler,
        .locals_dict = (mp_obj_dict_t *)&ev3dev_Font_locals_dict,
    },
    .attr_dict = ev3dev_Font_attr_dict,
    .attr_dict_size = MP_ARRAY_SIZE(ev3dev_Font_attr_dict),
};

GrxFont *pb_ev3dev_Font_obj_get_font(mp_const_obj_t obj) {
    if (!mp_obj_is_type(obj, &pb_type_ev3dev_Font.type)) {
        mp_raise_TypeError(MP_ERROR_TEXT("Requires Font object"));
    }
    ev3dev_Font_obj_t *self = MP_OBJ_TO_PTR(obj);
    return self->font;
}
