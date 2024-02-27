// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PYBRICKS_TYPE_BUTTON_H
#define PYBRICKS_INCLUDED_PYBRICKS_TYPE_BUTTON_H

#if PYBRICKS_PY_PARAMETERS

#if PYBRICKS_PY_PARAMETERS_BUTTON

#include "py/mpconfig.h"
#include "py/obj.h"

// Dynamically allocated button object.
typedef struct _pb_obj_button_t {
    mp_obj_base_t base;
    qstr name;
} pb_obj_button_t;

extern const pb_obj_button_t pb_type_button;

mp_obj_t pb_type_button_new(qstr name);
pbio_button_flags_t pb_type_button_get_button_flag(mp_obj_t obj);

typedef mp_obj_t (*pb_type_button_get_pressed_t)(void);
mp_obj_t pb_type_button_pressed_hub_single_button(void);

#endif // PYBRICKS_PY_PARAMETERS_BUTTON

#endif // PYBRICKS_PY_PARAMETERS

#endif // PYBRICKS_INCLUDED_PYBRICKS_TYPE_BUTTON_H
