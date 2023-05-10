// SPDX-License-Identifier: MIT
// Copyright (c) 2023 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_BUFFERED_STREAM

#include <assert.h>

#include <lwrb/lwrb.h>

#include "py/mperrno.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "pybricks/common.h"
#include "pybricks/util_mp/pb_kwarg_helper.h"

typedef struct {
    mp_obj_base_t base;
    lwrb_t *read_buf;
    // REVISIT: do we actually need a write buffer if we combine write and drain?
    lwrb_t *write_buf;
    void* context;
    pb_type_BufferedStream_notify_t notify;
    pb_type_BufferedStream_close_t close;
} pb_type_BufferedStream_t;

/**
 * Creates a new instance of a BufferedStream object.
 *
 * @param [in]  read_buf    The ring buffer used by read methods.
 * @param [in]  write_buf   The ring buffer used by write methods.
 * @param [in]  context     A user-defined context passed to callbacks.
 * @param [in]  notify      A callback function that is called when the read
 *                          or write buffer is modified.
 * @param [in]  close       A callback that is called when the stream is closed
 *                          or finalized.
 */
mp_obj_t pb_type_BufferedStream_obj_new(lwrb_t *read_buf, lwrb_t *write_buf, void* context,
    pb_type_BufferedStream_notify_t notify, pb_type_BufferedStream_close_t close) {

    assert(read_buf);
    assert(write_buf);
    assert(context);
    assert(notify);
    assert(close);

    // Extra byte for each buf is required by ring buffer implementation.
    pb_type_BufferedStream_t *self = m_new_obj_with_finaliser(pb_type_BufferedStream_t);
    self->base.type = &pb_type_BufferedStream;
    self->read_buf = read_buf;
    self->write_buf = write_buf;
    self->context = context;
    self->notify = notify;
    self->close = close;

    return MP_OBJ_FROM_PTR(self);
}

/**
 * Reads data from the read buffer.
 *
 * Semantics are similar to ``asyncio.streams.StreamReader.readexactly()``
 * without ``IncompleteReadError`` or EOF support.
 *
 * @param [in]  self_in     The BufferedStream instance object.
 * @param [in]  n_in        The number of bytes to read.
 * @returns                 A bytes object containing the bytes read.
 */
STATIC mp_obj_t pb_type_BufferedStream_read(mp_obj_t self_in, mp_obj_t n_in) {
    pb_type_BufferedStream_t *self = MP_OBJ_FROM_PTR(self_in);

    if (!self->close) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("stream is closed"));
    }

    mp_int_t n = mp_obj_get_int(n_in);

    if (n < 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("n must be >= 1"));
    }

    // TODO: make this blocking/async to read more than one buffer size.

    if ((size_t)n > self->read_buf->size - 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("n must be < buf len"));
    }

    size_t available = lwrb_get_full(self->read_buf);

    // No partial reads - either read size bytes or error.
    if (available < (size_t)n) {
        // in blocking/async version, we will wait/yield instead of raising
        mp_raise_OSError(MP_EWOULDBLOCK);
    }

    vstr_t vstr;
    vstr_init_len(&vstr, n);

    lwrb_read(self->read_buf, vstr.buf, vstr.len);

    if (self->notify) {
        self->notify(self->context);
    }

    return mp_obj_new_bytes_from_vstr(&vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pb_type_BufferedStream_read_obj, pb_type_BufferedStream_read);

/**
 * Reads one line of text from the stream.
 *
 * This requires stream data to be UTF-8 encoded and newlines are ``\n``.
 *
 * Semantics are similar to ``asyncio.streams.StreamReader.readline()`` but
 * the newline character is stripped rather than returned and no EOF support.
 *
 * @param [in]  self_in     The BufferedStream instance object.
 * @returns                 A str object containing the line read.
 */
STATIC mp_obj_t pb_type_BufferedStream_read_line(mp_obj_t self_in) {
    // Use lwrb_peek() to find the newline without moving the read pointer.
    // Use lwrb_skip() to avoid copying twice.
    // How to handle partial reads, e.g. where buffer is full and there is no
    // newline yet?
    mp_raise_NotImplementedError(MP_ERROR_TEXT("TODO"));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_BufferedStream_read_line_obj, pb_type_BufferedStream_read_line);

/**
 * Writes data to the stream.
 *
 * Semantics are similar to asyncio.StreamWriter.write().
 *
 * TODO: also combine asyncio.StreamWriter.drain(). Currently this just fails
 * if the buffer can't hold all of the data.
 */
STATIC mp_obj_t pb_type_BufferedStream_write(mp_obj_t self_in, mp_obj_t data_in) {
    pb_type_BufferedStream_t *self = MP_OBJ_FROM_PTR(self_in);

    if (!self->close) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("stream is closed"));
    }

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);

    // TODO: make this blocking/async to write more than one buffer size.

    if (bufinfo.len > self->write_buf->size - 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("data too big for buf"));
    }

    size_t available = lwrb_get_free(self->write_buf);

    // No partial writes - either write size bytes or error.
    if (available < bufinfo.len) {
        // in blocking/async version, we will wait/yield instead of raising
        mp_raise_OSError(MP_EWOULDBLOCK);
    }

    size_t written = lwrb_write(self->write_buf, bufinfo.buf, bufinfo.len);

    if (self->notify) {
        self->notify(self->context);
    }

    // TODO: wait for buffer to drain (and have wait=False similar to motors?)

    // REVISIT: if we don't allow partial writes do we really need to return this?
    return mp_obj_new_int(written);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pb_type_BufferedStream_write_obj, pb_type_BufferedStream_write);

/**
 * Writes one line of text from the stream.
 *
 * Semantics are similar to ``asyncio.streams.StreamWriter.writelines()`` but
 * only takes one line.
 *
 * @param [in]  self_in     The BufferedStream instance object.
 * @returns                 A str object containing the line read.
 */
STATIC mp_obj_t pb_type_BufferedStream_write_line(mp_obj_t self_in, mp_obj_t line_in) {
    // This can share most code with the write() method since the line arg does
    // not strictly need to be a str object and str already implements the buffer
    // protocol. We just need to make sure to also write the newline before
    // calling notify() to try to avoid splitting data into two packets on some
    // transports.
    mp_raise_NotImplementedError(MP_ERROR_TEXT("TODO"));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pb_type_BufferedStream_write_line_obj, pb_type_BufferedStream_write_line);

/**
 * Closes the stream and frees any OS resources.
 *
 * @param [in]  self_in     The BufferedStream instance object.
 */
STATIC mp_obj_t pb_type_BufferedStream_close(mp_obj_t self_in) {
    pb_type_BufferedStream_t *self = MP_OBJ_FROM_PTR(self_in);

    if (self->close) {
        self->close(self->context);
    }

    self->read_buf = NULL;
    self->write_buf = NULL;
    self->context = NULL;
    self->notify = NULL;
    self->close = NULL;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pb_type_BufferedStream_close_obj, pb_type_BufferedStream_close);

STATIC const mp_rom_map_elem_t pb_type_BufferedStream_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&pb_type_BufferedStream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_line), MP_ROM_PTR(&pb_type_BufferedStream_read_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&pb_type_BufferedStream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_line), MP_ROM_PTR(&pb_type_BufferedStream_write_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pb_type_BufferedStream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_type_BufferedStream_close_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_BufferedStream_locals_dict, pb_type_BufferedStream_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    pb_type_BufferedStream,
    MP_QSTR_BufferedStream,
    MP_TYPE_FLAG_NONE,
    locals_dict, &pb_type_BufferedStream_locals_dict);

#endif // PB_PY_COMMON_BUFFERED_STREAM
