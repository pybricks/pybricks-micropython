// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#ifndef PYBRICKS_INCLUDED_PBOBJ_H
#define PYBRICKS_INCLUDED_PBOBJ_H

#include <stdbool.h>
#include <stdint.h>

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

// Get absolute value if object is not none, else return default
mp_int_t pb_obj_get_default_abs_int(mp_obj_t obj, mp_int_t default_val);

// Get base instance if object is instance of subclass of type
mp_obj_t pb_obj_get_base_class_obj(mp_obj_t obj, const mp_obj_type_t *type);

// Raise error on unexpected type
void pb_assert_type(mp_obj_t obj, const mp_obj_type_t *type);

/**
 * Key/value pair for an attribute table lookup dictionary that maps an
 * attribute name to its metadata.
 */
typedef struct {
    /** The name of the attribute (qstr). */
    uint16_t name;
    /** The offset of the attribute's mp_obj_t in the object instance structure. */
    uint8_t offset;
    /** Indicates if the attribute can be read. */
    bool readable : 1;
    /** Indicates if the attribute can be written. */
    bool writeable : 1;
    /** Indicates if the attribute can be deleted. */
    bool deletable : 1;
} pb_attr_dict_entry_t;

// Generic entry of the attributes dictionary.
#define PB_DEFINE_CONST_ATTR(name_, type, field, r, w, d) {     \
        .name = (name_),                                        \
        .offset = offsetof(type, field),                        \
        .readable = (r),                                        \
        .writeable = (w),                                       \
        .deletable = (d),                                       \
}

// Read-only entry of the attributes dictionary. Dedicated macro because we use it so often.
#define PB_DEFINE_CONST_ATTR_RO(name, type, field) \
    PB_DEFINE_CONST_ATTR(name, type, field, true, false, false)

// Points to attributes dictionary. Must be the last entry of locals_dict.
#define PB_ATTRIBUTE_TABLE(table) { MP_ROM_QSTR(MP_QSTRnumber_of + MP_ARRAY_SIZE(table)), MP_ROM_PTR(table) }

// Attribute handler for any object that has an attribute dictionary.
void pb_attribute_handler(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

#endif // PYBRICKS_INCLUDED_PBOBJ_H
