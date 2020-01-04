// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#ifndef PYBRICKS_INCLUDED_BRICKS_EV3DEV_TYPES_H
#define PYBRICKS_INCLUDED_BRICKS_EV3DEV_TYPES_H

#include <grx-3.0.h>

#include "py/obj.h"

// class Font

// IMPORTANT: pb_type_ev3dev_Font_init() must be called before using
// pb_type_ev3dev_Font or pb_const_ev3dev_font_DEFAULT. It is safe to call
// pb_type_ev3dev_Font_init() more than once.
extern const mp_obj_type_t pb_type_ev3dev_Font;
#define pb_const_ev3dev_font_DEFAULT (MP_OBJ_FROM_PTR(&pb_const_ev3dev_Font_DEFAULT_obj))
extern const struct _ev3dev_Font_obj_t pb_const_ev3dev_Font_DEFAULT_obj;
void pb_type_ev3dev_Font_init();
GrxFont *pb_ev3dev_Font_obj_get_font(mp_const_obj_t obj);


// class Screen

const mp_obj_type_t pb_type_ev3dev_Screen;

#endif // PYBRICKS_INCLUDED_BRICKS_EV3DEV_TYPES_H
