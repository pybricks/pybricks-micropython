// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

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

/**
 * Micropython type object struct that is extened to include an attribute map.
 *
 * This is used in conjunction with pb_attribute_handler() to provide simple
 * attributes similar to regular Python attributes and can optionally be
 * read-only.
 */
typedef struct {
    /** The base type structure. */
    mp_obj_type_t type;
    /**
     * The attribute lookup table (similar to type.locals_dict). This maps
     * attribute names (qstrs) to an offset in the object instance struct
     * where the value of the attribute is stored.
     */
    const pb_attr_dict_entry_t *attr_dict;
    /**
     * The number of entries in attr_dict.
     */
    uint8_t attr_dict_size;
} pb_obj_with_attr_type_t;

/**
 * Micropython attribute handler for pb_obj_with_attr_type_t.
 *
 * Assign this to .attr when defining the type.
 */
void pb_attribute_handler(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

#endif // PYBRICKS_INCLUDED_PBOBJ_H
