// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PBOBJ_H
#define PYBRICKS_INCLUDED_PBOBJ_H

#include <fixmath.h>

#include <pbio/color.h>

#include "py/obj.h"

// like mp_obj_get_int() but also allows float
#if MICROPY_PY_BUILTINS_FLOAT
mp_int_t pb_obj_get_int(mp_obj_t arg);
#else
#define pb_obj_get_int mp_obj_get_int
#endif

mp_int_t pb_obj_get_positive_int(mp_obj_t arg);
mp_int_t pb_obj_get_pct(mp_obj_t arg);
mp_int_t pb_obj_get_hue(mp_obj_t arg);
void pb_obj_get_hsv(mp_obj_t arg, pbio_color_hsv_t *hsv);

// like mp_obj_new_int / mp_obj_new_float to create object as a ratio of two integers
mp_obj_t pb_obj_new_fraction(int32_t numerator, int32_t denominator);

fix16_t pb_obj_get_fix16(mp_obj_t arg);

// Get value if object is not none, else return default
mp_int_t pb_obj_get_default_int(mp_obj_t obj, mp_int_t default_val);

// Get base instance if object is instance of subclass of type
mp_obj_t pb_obj_get_base_class_obj(mp_obj_t obj, const mp_obj_type_t *type);

// Raise error on unexpected type
void pb_assert_type(mp_obj_t obj, const mp_obj_type_t *type);

// Read/write/delete flags for constant attributes in custom types
enum {
    PB_ATTR_READABLE = (1 << 16),  /**< Attribute can be read */
    PB_ATTR_WRITABLE = (1 << 17),  /**< Attribute can be assigned a new object */
    PB_ATTR_DELETABLE = (1 << 18), /**< Attribute can be deleted with del */
    PB_ATTR_OFFSET_MASK = 0xFFFF,  /**< Internal use. Mask for the offset value stored in the LSB bytes */
};

// Generic entry of the attributes dictionary.
#define PB_DEFINE_CONST_ATTR(type, name, field, flags) \
    { MP_ROM_QSTR(name), MP_ROM_PTR(&(mp_uint_t) {offsetof(type, field) | (flags)}) }

// Read-only entry of the attributes dictionary. Dedicated macro because we use it so often.
#define PB_DEFINE_CONST_ATTR_RO(type, name, field) \
    PB_DEFINE_CONST_ATTR(type, name, field, PB_ATTR_READABLE)

// Points to attributes dictionary. MUST be first entry of locals_dict.
#define PB_ATTRIBUTE_TABLE(offset_dict) { MP_ROM_QSTR(MP_QSTR__attr), MP_ROM_PTR(&offset_dict)}

// Attribute handler for any object that has an attribute dictionary.
void pb_attribute_handler(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

#endif // PYBRICKS_INCLUDED_PBOBJ_H
