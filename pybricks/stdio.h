// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

#ifndef _PYBRICKS_STDIO_H_
#define _PYBRICKS_STDIO_H_

#include "py/mpconfig.h"

#if PYBRICKS_PY_STDIO

#include "py/obj.h"
#include "py/stream.h"

typedef struct {
    mp_obj_base_t base;
    mp_obj_t binary_stream_obj;
    const mp_stream_p_t *binary_stream_p;
} pb_text_io_wrapper_t;

extern const mp_obj_type_t pb_text_io_wrapper_obj_type;
extern const mp_obj_dict_t pb_stdio_locals_dict;

extern const pb_text_io_wrapper_t pb_bluetooth_stdio_wrapper_obj;
extern const pb_text_io_wrapper_t pb_usb_stdio_wrapper_obj;

#endif // PYBRICKS_PY_STDIO

#endif // _PYBRICKS_STDIO_H_
