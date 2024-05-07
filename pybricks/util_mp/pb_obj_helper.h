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

mp_int_t pb_obj_get_scaled_int(mp_obj_t arg, mp_uint_t scale);
mp_int_t pb_obj_get_positive_int(mp_obj_t arg);
mp_int_t pb_obj_get_pct(mp_obj_t arg);
mp_int_t pb_obj_get_hue(mp_obj_t arg);
void pb_obj_get_hsv(mp_obj_t arg, pbio_color_hsv_t *hsv);

// like mp_obj_new_int / mp_obj_new_float to create object as a ratio of two integers
mp_obj_t pb_obj_new_fraction(int32_t numerator, int32_t denominator);

// Get absolute value if object is not none, else return default
mp_int_t pb_obj_get_default_abs_int(mp_obj_t obj, mp_int_t default_val);

// Tests if an object is a tuple or list
bool pb_obj_is_array(mp_obj_t obj_in);

// Get array of percentages from single value or tuple
void pb_obj_get_pct_or_array(mp_obj_t obj_in, size_t num, int8_t *values);

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
} pb_attr_dict_entry_t;

// pb_attr_dict_entry_t arrays must be zero-terminated
#define PB_ATTR_DICT_SENTINEL {}

// Generic entry of the attributes dictionary.
#define PB_DEFINE_CONST_ATTR_RO(name_, type, field) {           \
        .name = (name_),                                        \
        .offset = offsetof(type, field),                        \
}

// Attribute handler for any object that has an attribute dictionary assigned
// to the mp_obj_type_t protocol slot.
void pb_attribute_handler(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

mp_obj_t pb_function_import_helper(qstr module_name, qstr function_name);

#endif // PYBRICKS_INCLUDED_PBOBJ_H
