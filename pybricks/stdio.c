// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// Pybricks custom implementation of replaceable stdio streams for MicroPython.
// This is used for sys.stdin, sys.stdout, and sys.stderr.

#include "py/mpconfig.h"
#include "py/runtime.h"
#include "py/stream.h"

#include <pybricks/stdio.h>

#if PYBRICKS_PY_STDIO

// This text wrapper class wraps a binary stream and handles line endings so
// that it interacts properly when connected to a terminal program.

// REVIST: Would be nice to implement proper unicode support.

static void pb_text_io_wrapper_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    pb_text_io_wrapper_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        if (attr == MP_QSTR_buffer) {
            dest[0] = self->binary_stream_obj;
        } else {
            // Continue lookup in locals_dict.
            dest[1] = MP_OBJ_SENTINEL;
        }
    }
}

static mp_uint_t pb_text_io_wrapper_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    pb_text_io_wrapper_t *self = MP_OBJ_TO_PTR(self_in);

    for (uint i = 0; i < size; i++) {
        byte c;
        mp_uint_t ret = self->binary_stream_p->read(self->binary_stream_obj, &c, 1, errcode);
        if (ret == MP_STREAM_ERROR) {
            return MP_STREAM_ERROR;
        }
        if (ret == 0) {
            return i;
        }
        if (c == '\r') {
            c = '\n';
        }
        ((byte *)buf)[i] = c;
    }

    return size;
}

static mp_uint_t pb_text_io_wrapper_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    pb_text_io_wrapper_t *self = MP_OBJ_TO_PTR(self_in);

    const char *last = buf;
    const char *str = buf;
    mp_uint_t len = size;
    mp_uint_t ret;

    while (len--) {
        if (*str == '\n') {
            if (str > last) {
                ret = self->binary_stream_p->write(self->binary_stream_obj, last, str - last, errcode);
                if (ret == MP_STREAM_ERROR) {
                    return MP_STREAM_ERROR;
                }
                if (ret == 0) {
                    return size - len;
                }
            }

            ret = self->binary_stream_p->write(self->binary_stream_obj, "\r\n", 2, errcode);
            if (ret == MP_STREAM_ERROR) {
                return MP_STREAM_ERROR;
            }
            if (ret == 0) {
                return size - len;
            }

            str++;
            last = str;
        } else {
            str++;
        }
    }

    if (str > last) {
        ret = self->binary_stream_p->write(self->binary_stream_obj, last, str - last, errcode);
        if (ret == MP_STREAM_ERROR) {
            return MP_STREAM_ERROR;
        }
        if (ret == 0) {
            return size - len;
        }
    }

    return size;
}

static mp_uint_t pb_text_io_wrapper_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    pb_text_io_wrapper_t *self = MP_OBJ_TO_PTR(self_in);

    // IOCTLS can just be passed through to the underlying binary stream.
    return self->binary_stream_p->ioctl(self->binary_stream_obj, request, arg, errcode);
}

static const mp_stream_p_t pb_text_io_wrapper_stream_p = {
    .read = pb_text_io_wrapper_read,
    .write = pb_text_io_wrapper_write,
    .ioctl = pb_text_io_wrapper_ioctl,
    .is_text = true,
};

// This dict is shared with all Pybricks stdio-like objects.
static const mp_rom_map_elem_t pb_stdio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj)},
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj) },
};
MP_DEFINE_CONST_DICT(pb_stdio_locals_dict, pb_stdio_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    pb_text_io_wrapper_obj_type,
    MP_QSTR_TextIOWrapper,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    attr, pb_text_io_wrapper_attr,
    protocol, &pb_text_io_wrapper_stream_p,
    locals_dict, &pb_stdio_locals_dict);

#endif // PYBRICKS_PY_STDIO
