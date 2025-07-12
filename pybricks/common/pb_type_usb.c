
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_USB

#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/stdio.h>

typedef struct {
    mp_obj_base_t base;
} pb_obj_USB_t;

static const mp_rom_map_elem_t common_USB_locals_dict_table[] = {
    #if PYBRICKS_PY_STDIO
    { MP_ROM_QSTR(MP_QSTR_io), MP_ROM_PTR(&pb_usb_stdio_wrapper_obj) },
    #endif
};
static MP_DEFINE_CONST_DICT(common_USB_locals_dict, common_USB_locals_dict_table);

static MP_DEFINE_CONST_OBJ_TYPE(pb_type_USB,
    MP_QSTR_USB,
    MP_TYPE_FLAG_NONE,
    locals_dict, &common_USB_locals_dict);

static const pb_obj_USB_t pb_type_USB_instance = {
    .base = { .type = &pb_type_USB },
};

/**
 * Creates a new instance of the USB class.
 *
 * @returns A signle instance of the USB class.
 */
mp_obj_t pb_type_USB_new(void) {
    return MP_OBJ_FROM_PTR(&pb_type_USB_instance);
}

#endif // PYBRICKS_PY_COMMON_USB
