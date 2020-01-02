// SPDX-License-Identifier
// Copyright (c) 2019 David Lechner

// class Screen

#include <grx-3.0.h>

#include <pbio/light.h>

#include "py/mpconfig.h"
#include "py/misc.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "pb_ev3dev_types.h"
#include "modparameters.h"
#include "pbkwarg.h"

typedef struct _ev3dev_Screen_obj_t {
    mp_obj_base_t base;
    mp_obj_t width;
    mp_obj_t height;
    gboolean cleared;
    gboolean initialized;
} ev3dev_Screen_obj_t;

STATIC ev3dev_Screen_obj_t ev3dev_Screen_singleton;

// map Pybricks color enum to GRX color value using standard web CSS values
STATIC GrxColor map_color(mp_obj_t *obj) {
    pbio_light_color_t color = pb_type_enum_get_value(obj, &pb_enum_type_Color);

    switch (color) {
    case PBIO_LIGHT_COLOR_NONE:
        return GRX_COLOR_NONE;
    case PBIO_LIGHT_COLOR_BLACK:
        return GRX_COLOR_BLACK;
    case PBIO_LIGHT_COLOR_BLUE:
        return grx_color_get(0, 0, 255);
    case PBIO_LIGHT_COLOR_GREEN:
        return grx_color_get(0, 128, 0);
    case PBIO_LIGHT_COLOR_YELLOW:
        return grx_color_get(255, 255, 0);
    case PBIO_LIGHT_COLOR_RED:
        return grx_color_get(255, 0, 0);
    case PBIO_LIGHT_COLOR_WHITE:
        return GRX_COLOR_WHITE;
    case PBIO_LIGHT_COLOR_BROWN:
        return grx_color_get(165, 42, 42);
    case PBIO_LIGHT_COLOR_ORANGE:
        return grx_color_get(255, 165, 0);
    case PBIO_LIGHT_COLOR_PURPLE:
        return grx_color_get(128, 0, 128);
    }
    return grx_color_get_black();
}

STATIC mp_obj_t ev3dev_Screen_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    if (!ev3dev_Screen_singleton.initialized) {
        ev3dev_Screen_singleton.base.type = &pb_type_ev3dev_Screen;
        ev3dev_Screen_singleton.width = MP_OBJ_NEW_SMALL_INT(grx_get_screen_width());
        ev3dev_Screen_singleton.height = MP_OBJ_NEW_SMALL_INT(grx_get_screen_height());
        ev3dev_Screen_singleton.initialized = TRUE;
    }
    return &ev3dev_Screen_singleton;
}

STATIC mp_obj_t ev3dev_Screen_clear(mp_obj_t self_in) {
    grx_clear_screen(GRX_COLOR_WHITE);
    ev3dev_Screen_singleton.cleared = TRUE;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(ev3dev_Screen_clear_obj, ev3dev_Screen_clear);

// Ensure that screen has been cleared before we start drawing anything else
STATIC void clear_once(void) {
    if (!ev3dev_Screen_singleton.cleared) {
        grx_clear_screen(GRX_COLOR_WHITE);
        ev3dev_Screen_singleton.cleared = TRUE;
    }
}

STATIC mp_obj_t ev3dev_Screen_draw_pixel(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Screen_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_DEFAULT_ENUM(color, pb_const_black)
    );

    (void)self; // unused
    mp_int_t _x = mp_obj_get_int(x);
    mp_int_t _y = mp_obj_get_int(y);
    GrxColor _color = map_color(color);

    clear_once();
    grx_draw_pixel(_x, _y, _color);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Screen_draw_pixel_obj, 0, ev3dev_Screen_draw_pixel);

STATIC mp_obj_t ev3dev_Screen_draw_line(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Screen_obj_t, self,
        PB_ARG_REQUIRED(x1),
        PB_ARG_REQUIRED(y1),
        PB_ARG_REQUIRED(x2),
        PB_ARG_REQUIRED(y2),
        PB_ARG_DEFAULT_ENUM(color, pb_const_black)
    );

    (void)self; // unused
    mp_int_t _x1 = mp_obj_get_int(x1);
    mp_int_t _y1 = mp_obj_get_int(y1);
    mp_int_t _x2 = mp_obj_get_int(x2);
    mp_int_t _y2 = mp_obj_get_int(y2);
    GrxColor _color = map_color(color);

    clear_once();
    grx_draw_line(_x1, _y1, _x2, _y2, _color);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Screen_draw_line_obj, 0, ev3dev_Screen_draw_line);

STATIC mp_obj_t ev3dev_Screen_draw_box(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Screen_obj_t, self,
        PB_ARG_REQUIRED(x1),
        PB_ARG_REQUIRED(y1),
        PB_ARG_REQUIRED(x2),
        PB_ARG_REQUIRED(y2),
        PB_ARG_DEFAULT_INT(r, 0),
        PB_ARG_DEFAULT_FALSE(fill),
        PB_ARG_DEFAULT_ENUM(color, pb_const_black)
    );

    (void)self; // unused
    mp_int_t _x1 = mp_obj_get_int(x1);
    mp_int_t _y1 = mp_obj_get_int(y1);
    mp_int_t _x2 = mp_obj_get_int(x2);
    mp_int_t _y2 = mp_obj_get_int(y2);
    mp_int_t _r = mp_obj_get_int(r);
    GrxColor _color = map_color(color);

    clear_once();
    if (mp_obj_is_true(fill)) {
        if (_r > 0) {
            grx_draw_filled_rounded_box(_x1, _y1, _x2, _y2, _r, _color);
        }
        else {
            grx_draw_filled_box(_x1, _y1, _x2, _y2, _color);
        }
    }
    else {
        if (_r > 0) {
            grx_draw_rounded_box(_x1, _y1, _x2, _y2, _r, _color);
        }
        else {
            grx_draw_box(_x1, _y1, _x2, _y2, _color);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Screen_draw_box_obj, 0, ev3dev_Screen_draw_box);

STATIC mp_obj_t ev3dev_Screen_draw_circle(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        ev3dev_Screen_obj_t, self,
        PB_ARG_REQUIRED(x),
        PB_ARG_REQUIRED(y),
        PB_ARG_DEFAULT_INT(r, 0),
        PB_ARG_DEFAULT_FALSE(fill),
        PB_ARG_DEFAULT_ENUM(color, pb_const_black)
    );

    (void)self; // unused
    mp_int_t _x = mp_obj_get_int(x);
    mp_int_t _y = mp_obj_get_int(y);
    mp_int_t _r = mp_obj_get_int(r);
    GrxColor _color = map_color(color);

    clear_once();
    if (mp_obj_is_true(fill)) {
        grx_draw_filled_circle(_x, _y, _r, _color);
    }
    else {
        grx_draw_circle(_x, _y, _r, _color);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(ev3dev_Screen_draw_circle_obj, 0, ev3dev_Screen_draw_circle);

STATIC const mp_rom_map_elem_t ev3dev_Screen_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_clear),       MP_ROM_PTR(&ev3dev_Screen_clear_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_draw_pixel),  MP_ROM_PTR(&ev3dev_Screen_draw_pixel_obj)               },
    { MP_ROM_QSTR(MP_QSTR_draw_line),   MP_ROM_PTR(&ev3dev_Screen_draw_line_obj)                },
    { MP_ROM_QSTR(MP_QSTR_draw_box),    MP_ROM_PTR(&ev3dev_Screen_draw_box_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_draw_circle), MP_ROM_PTR(&ev3dev_Screen_draw_circle_obj)              },
    { MP_ROM_QSTR(MP_QSTR_WIDTH),       MP_ROM_ATTRIBUTE_OFFSET(ev3dev_Screen_obj_t, width)     },
    { MP_ROM_QSTR(MP_QSTR_HEIGHT),      MP_ROM_ATTRIBUTE_OFFSET(ev3dev_Screen_obj_t, height)    },
};
STATIC MP_DEFINE_CONST_DICT(ev3dev_Screen_locals_dict, ev3dev_Screen_locals_dict_table);

const mp_obj_type_t pb_type_ev3dev_Screen = {
    { &mp_type_type },
    .name = MP_QSTR_Screen,
    .make_new = ev3dev_Screen_make_new,
    .locals_dict = (mp_obj_dict_t*)&ev3dev_Screen_locals_dict,
};
