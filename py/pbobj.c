// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <pbio/error.h>

#include <fixmath.h>

#include "pberror.h"

#include "py/objstr.h"
#include "py/mpconfig.h"
#include "py/obj.h"


#if MICROPY_PY_BUILTINS_FLOAT
mp_int_t pb_obj_get_int(mp_obj_t arg) {
    if (mp_obj_is_float(arg)) {
        return (mp_int_t)mp_obj_get_float(arg);
    }
    return mp_obj_get_int(arg);
}
#endif

fix16_t pb_obj_get_fix16(mp_obj_t arg) {
#if MICROPY_PY_BUILTINS_FLOAT
    if (mp_obj_is_float(arg)) {
        return fix16_from_float((float) mp_obj_get_float(arg));
    }
#endif
    return fix16_from_int(mp_obj_get_int(arg));
}

mp_int_t pb_obj_get_default_int(mp_obj_t obj, mp_int_t default_val) {
    return obj == mp_const_none ? default_val : mp_obj_get_int(obj);
}

bool unpack_byte_arg(mp_obj_t arg, uint8_t **bytes, size_t *len) {

    // For prealocated byte arrays, we can just get a pointer to the data
    // And MicroPython will take care of cleaning it as needed.
    if (mp_obj_is_str_or_bytes(arg)) {
        GET_STR_DATA_LEN(arg, string, string_len);
        *bytes = (uint8_t *) string;
        *len = string_len;
        // We don't need to clean this up, so return false
        return false;
    }

    mp_obj_t *data;

    // Argument is a single int
    if (mp_obj_is_int(arg)) {
        *len = 1;
        data = &arg;
    }
    // Argument is a tuple/list, so unpack them
    else if (mp_obj_is_type(arg, &mp_type_tuple) || mp_obj_is_type(arg, &mp_type_list)) {
        mp_obj_get_array(arg, len, &data);
    }
    else {
        pb_assert(PBIO_ERROR_INVALID_ARG);
    }

    // If the length is zero, do no allocate memory and do not clean it up
    if (*len == 0) {
        return false;
    }

    // Allocate the byte buffer
    *bytes = m_malloc(*len);
    if (*bytes == NULL) {
        pb_assert(PBIO_ERROR_FAILED);
    }

    pbio_error_t err = PBIO_SUCCESS;

    // Extract the bytes from the data
    for (size_t i = 0; i < *len; i++) {
        // If it not an int, stop.
        if (!mp_obj_is_int(data[i])) {
            err = PBIO_ERROR_INVALID_ARG;
            break;
        }
        // If value is out of range, stop
        mp_int_t val = mp_obj_get_int(data[i]);
        if (val < 0 || val > 255) {
            err = PBIO_ERROR_INVALID_ARG;
            break;
        }
        // All is fine, so store the byte
        (*bytes)[i] = val;
    }

    // Clean up if we failed to get the data
    if (err != PBIO_SUCCESS) {
        m_free(*bytes, *len);
        pb_assert(err);
        return false;
    }

    // Success. We do need to clean up after use, so return true.
    return true;
}
