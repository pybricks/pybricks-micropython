// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2023 The Pybricks Authors

#include <pbio/color.h>
#include <pbio/error.h>

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objtype.h"
#include "py/runtime.h"

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

#if MICROPY_PY_BUILTINS_FLOAT
mp_int_t pb_obj_get_int(mp_obj_t arg) {
    if (mp_obj_is_float(arg)) {
        return (mp_int_t)mp_obj_get_float(arg);
    }
    return mp_obj_get_int(arg);
}
#endif

/**
 * Gets an upscaled integer representing a floating point value.
 *
 * If @p arg cannot be converted to a float or integer, an exception is raised.
 *
 * @param arg [in]  A MicroPython object
 * @return          An integer
 */
mp_int_t pb_obj_get_scaled_int(mp_obj_t arg, mp_uint_t scale) {
    #if MICROPY_PY_BUILTINS_FLOAT
    if (mp_obj_is_float(arg)) {
        return (mp_int_t)(mp_obj_get_float(arg) * scale);
    }
    #endif
    return mp_obj_get_int(arg) * scale;
}

/**
 * Gets a positive integer value.
 *
 * If @p arg cannot be converted to an integer, an exception is raised.
 * If @p arg is less than 0, it is truncated to 0.
 *
 * @param arg [in]  A MicroPython object
 * @return          A positive integer
 */
mp_int_t pb_obj_get_positive_int(mp_obj_t arg) {
    mp_int_t val = pb_obj_get_int(arg);
    if (val < 0) {
        return 0;
    }
    return val;
}

/**
 * Gets a percentage value.
 *
 * If @p arg cannot be converted to an integer, an exception is raised.
 * If @p arg is less than 0, it is truncated to 0.
 * If @p arg is greater than 100, it is truncated to 100.
 *
 * @param arg [in]  A MicroPython object
 * @return          An integer in the range 0 to 100
 */
mp_int_t pb_obj_get_pct(mp_obj_t arg) {
    mp_int_t val = pb_obj_get_positive_int(arg);
    if (val > 100) {
        return 100;
    }
    return val;
}

/**
 * Gets a HSV colorspace hue value.
 *
 * If @p arg cannot be converted to an integer, an exception is raised.
 * If @p arg is outside of the range of 0 to 359, it is treated as an angle
 * and is reduced to the matching angle between 0 and 359.
 *
 * @param arg [in]  A MicroPython object
 * @return          An integer in the range of 0 to 359.
 */
mp_int_t pb_obj_get_hue(mp_obj_t arg) {
    mp_int_t hue = pb_obj_get_int(arg) % 360;
    if (hue < 0) {
        return hue + 360;
    }
    return hue;
}

/**
 * Unpacks an object into a HSV struct.
 *
 * Raises an exception if @p arg is not subscriptable or does not have 3 elements.
 *
 * @param arg [in]  A MicroPython object
 * @param hsv [out] The HSV value
 */
void pb_obj_get_hsv(mp_obj_t arg, pbio_color_hsv_t *hsv) {
    hsv->h = pb_obj_get_hue(mp_obj_subscr(arg, MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_SENTINEL));
    hsv->s = pb_obj_get_pct(mp_obj_subscr(arg, MP_OBJ_NEW_SMALL_INT(1), MP_OBJ_SENTINEL));
    hsv->v = pb_obj_get_pct(mp_obj_subscr(arg, MP_OBJ_NEW_SMALL_INT(2), MP_OBJ_SENTINEL));
}

mp_obj_t pb_obj_new_fraction(int32_t numerator, int32_t denominator) {
    #if MICROPY_PY_BUILTINS_FLOAT
    return mp_obj_new_float(((mp_float_t)numerator) / ((mp_float_t)denominator));
    #else
    return mp_obj_new_int(numerator / denominator);
    #endif
}

mp_int_t pb_obj_get_default_abs_int(mp_obj_t obj, mp_int_t default_val) {
    if (obj == mp_const_none) {
        return default_val;
    }
    mp_int_t value = pb_obj_get_int(obj);
    return value > 0 ? value: -value;
}

mp_obj_t pb_obj_get_base_class_obj(mp_obj_t obj, const mp_obj_type_t *type) {

    // If it equals the base type then return as is
    if (mp_obj_is_type(obj, type)) {
        return obj;
    }
    // If it is an instance of a derived class, get base instance
    if (mp_obj_is_obj(obj) && mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t *)MP_OBJ_TO_PTR(obj))->type), MP_OBJ_FROM_PTR(type))) {
        return ((mp_obj_instance_t *)MP_OBJ_TO_PTR(obj))->subobj[0];
    }
    // On failure, say why we could not do it
    pb_assert_type(obj, type);
    return MP_OBJ_NULL;
}

void pb_assert_type(mp_obj_t obj, const mp_obj_type_t *type) {
    if (!mp_obj_is_type(obj, type)) {
        #if MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE
        // HACK: for some reason, GCC 10 LTO optimizes out pb_assert_type()
        // in the terse case. By adding an empty inline assembly statement,
        // we prevent the incorrect optimization.
        __asm__ ("");
        mp_raise_TypeError(NULL);
        #else
        mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT("can't convert %s to %s"),
            mp_obj_get_type_str(obj), qstr_str(type->name));
        #endif
    }
}

void pb_attribute_handler(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    const mp_obj_type_t *type = mp_obj_get_type(self_in);

    // type may have been subclassed
    while (MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, attr) != pb_attribute_handler) {
        type = type->base.type;

        if (type == &mp_type_type) {
            // If we get to here, we are at the base type of all types and
            // there was no parent type that supplied the attributes.In theory,
            // this should never happen.
            return;
        }
    }

    // Get number of attributes and reference to attributes array.
    const pb_attr_dict_entry_t *attr_dict = MP_OBJ_TYPE_GET_SLOT(type, protocol);
    const pb_attr_dict_entry_t *entry = NULL;

    // Look up the attribute offset. attr_dict is zero-terminated.
    for (int i = 0; attr_dict[i].name; i++) {
        if (attr_dict[i].name == attr) {
            entry = &attr_dict[i];
            break;
        }
    }

    if (entry == NULL) {
        // Attribute not found, continue lookup in locals dict.
        dest[1] = MP_OBJ_SENTINEL;
        return;
    }

    // Check if this is a read operation.
    if (dest[0] == MP_OBJ_NULL) {
        // Object is located at the base address plus the offset.
        dest[0] = *(mp_obj_t *)((char *)MP_OBJ_TO_PTR(self_in) + entry->offset);
    }

    // Write/delete not supported.
}
