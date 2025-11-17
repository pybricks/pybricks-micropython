// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_PARAMETERS_IMAGE

#include <pybricks/parameters.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>

#include <pbio/image.h>

#include <pbdrv/display.h>

extern const mp_obj_type_t pb_type_Font;

typedef struct _pb_type_Font_obj_t {
    mp_obj_base_t base;
    const pbio_font_t *font;
} pb_type_Font_obj_t;

const pb_type_Font_obj_t pb_type_Font_DEFAULT_obj = {
    {&pb_type_Font},
    // This will request the default font from the display driver.
    .font = NULL,
};

const pb_type_Font_obj_t pb_type_Font_TERMINUS_16_obj = {
    {&pb_type_Font},
    .font = &pbio_font_terminus_normal_16,
};

const pb_type_Font_obj_t pb_type_Font_LIBERATIONSANS_14_obj = {
    {&pb_type_Font},
    .font = &pbio_font_liberationsans_regular_14,
};

const pb_type_Font_obj_t pb_type_Font_MONO_8X5_8_obj = {
    {&pb_type_Font},
    .font = &pbio_font_mono_8x5_8,
};

static const pbio_font_t *get_font(pb_type_Font_obj_t *font_obj) {
    if (font_obj->font) {
        return font_obj->font;
    } else {
        pbio_image_t *display = pbdrv_display_get_image();
        return display->print_font;
    }
}

const pbio_font_t *pb_type_Font_get_font(mp_obj_t obj) {
    pb_assert_type(obj, &pb_type_Font);
    pb_type_Font_obj_t *font_obj = MP_OBJ_TO_PTR(obj);
    return get_font(font_obj);
}

static mp_obj_t pb_type_Font_text_width_height(bool width, mp_obj_t self_in, mp_obj_t text_in) {
    pb_type_Font_obj_t *self = MP_OBJ_TO_PTR(self_in);

    size_t text_len;
    const char *text = mp_obj_str_get_data(text_in, &text_len);

    pbio_image_rect_t rect;

    pbio_image_bbox_text(get_font(self), text, text_len, &rect);

    return mp_obj_new_int(width ? rect.width : rect.height);
}

static mp_obj_t pb_type_Font_text_width(mp_obj_t self_in, mp_obj_t text_in) {
    return pb_type_Font_text_width_height(true, self_in, text_in);
}
static MP_DEFINE_CONST_FUN_OBJ_2(pb_type_Font_text_width_obj, pb_type_Font_text_width);

static mp_obj_t pb_type_Font_text_height(mp_obj_t self_in, mp_obj_t text_in) {
    return pb_type_Font_text_width_height(false, self_in, text_in);
}
static MP_DEFINE_CONST_FUN_OBJ_2(pb_type_Font_text_height_obj, pb_type_Font_text_height);

static void pb_type_Font_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    // Read only
    if (dest[0] == MP_OBJ_NULL) {
        pb_type_Font_obj_t *self = MP_OBJ_TO_PTR(self_in);
        const pbio_font_t *font = get_font(self);
        switch (attr) {
            case MP_QSTR_family:
                dest[0] = mp_obj_new_str_from_cstr(font->family_name);
                return;
            case MP_QSTR_style:
                dest[0] = mp_obj_new_str_from_cstr(font->style_name);
                return;
            case MP_QSTR_width: {
                int width = 0;
                for (int i = 0; i <= font->last - font->first; i++) {
                    int gwidth = font->glyphs[i].advance;
                    if (gwidth > width) {
                        width = gwidth;
                    }
                }
                dest[0] = mp_obj_new_int(width);
                return;
            }
            case MP_QSTR_height:
                dest[0] = mp_obj_new_int(font->line_height);
                return;
        }
    }
    // Attribute not found, continue lookup in locals dict.
    dest[1] = MP_OBJ_SENTINEL;
}

// dir(pybricks.parameters.Font)
static const mp_rom_map_elem_t pb_type_Font_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_DEFAULT),           MP_ROM_PTR(&pb_type_Font_DEFAULT_obj) },
    { MP_ROM_QSTR(MP_QSTR_TERMINUS_16),       MP_ROM_PTR(&pb_type_Font_TERMINUS_16_obj) },
    { MP_ROM_QSTR(MP_QSTR_LIBERATIONSANS_14), MP_ROM_PTR(&pb_type_Font_LIBERATIONSANS_14_obj) },
    { MP_ROM_QSTR(MP_QSTR_MONO_8X5_8),        MP_ROM_PTR(&pb_type_Font_MONO_8X5_8_obj) },
    { MP_ROM_QSTR(MP_QSTR_text_width),        MP_ROM_PTR(&pb_type_Font_text_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_text_height),       MP_ROM_PTR(&pb_type_Font_text_height_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_Font_locals_dict, pb_type_Font_locals_dict_table);

// type(pybricks.parameters.Font)
MP_DEFINE_CONST_OBJ_TYPE(pb_type_Font,
    MP_QSTR_Font,
    MP_TYPE_FLAG_NONE,
    attr, pb_type_Font_attr,
    locals_dict, &pb_type_Font_locals_dict);

#endif // PYBRICKS_PY_PARAMETERS_IMAGE
