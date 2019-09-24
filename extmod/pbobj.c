// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

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

mp_int_t pb_obj_get_default_int(mp_obj_t obj, mp_int_t default_val) {
    return obj == mp_const_none ? default_val : mp_obj_get_int(obj);
}
