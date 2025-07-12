// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors
//
// Python stream object that uses Pybricks Profile stdio over Bluetooth.

#include <pbsys/bluetooth.h>
#include <pbsys/config.h>

#include <pybricks/stdio.h>
#include <pybricks/util_pb/pb_error.h>

#include "py/mpconfig.h"
#include "py/runtime.h"
#include "py/stream.h"

#if PYBRICKS_PY_STDIO && PBSYS_CONFIG_BLUETOOTH

typedef struct {
    mp_obj_base_t base;
} pb_stdio_obj_t;

static mp_uint_t pb_bluetooth_stdio_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    uint32_t remaining = size;

    for (;;) {
        uint32_t chunk = remaining;

        pbio_error_t err = pbsys_bluetooth_rx(buf, &chunk);
        if (err == PBIO_SUCCESS) {
            buf += chunk;
            remaining -= chunk;

            if (!remaining) {
                return size;
            }
        } else if (err == PBIO_ERROR_INVALID_OP) {
            // For backwards compatibility, don't raise error if not connected,
            // just block forever.
            // REVISIT, could add an attribute to control this behavior.
        } else if (err != PBIO_ERROR_AGAIN) {
            *errcode = pb_errcode_from_pbio_error(err);
            return MP_STREAM_ERROR;
        }

        MICROPY_EVENT_POLL_HOOK
    }
}

static mp_uint_t pb_bluetooth_stdio_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    uint32_t remaining = size;

    while (remaining) {
        uint32_t chunk = remaining;

        pbio_error_t err = pbsys_bluetooth_tx(buf, &chunk);
        if (err == PBIO_SUCCESS) {
            buf += chunk;
            remaining -= chunk;
        } else if (err == PBIO_ERROR_INVALID_OP) {
            // For backwards compatibility, if not connected, send to /dev/null.
            // REVISIT, could add an attribute to control this behavior.
            return size;
        } else if (err != PBIO_ERROR_AGAIN) {
            *errcode = pb_errcode_from_pbio_error(err);
            return MP_STREAM_ERROR;
        }

        MICROPY_EVENT_POLL_HOOK

    }

    return size - remaining;
}

static mp_uint_t pb_bluetooth_stdio_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    switch (request) {
        case MP_STREAM_POLL: {
            mp_uint_t ret = 0;

            if ((arg & MP_STREAM_POLL_RD) && pbsys_bluetooth_rx_get_available()) {
                ret |= MP_STREAM_POLL_RD;
            }

            if ((arg & MP_STREAM_POLL_WR) && pbsys_bluetooth_tx_is_idle()) {
                ret |= MP_STREAM_POLL_WR;
            }

            return ret;
        }
        case MP_STREAM_CLOSE:
            return 0;
        case MP_STREAM_FLUSH:
            while (!pbsys_bluetooth_tx_is_idle()) {
                MICROPY_EVENT_POLL_HOOK
            }
            return 0;
        default:
            break;
    }

    *errcode = MP_EINVAL;
    return MP_STREAM_ERROR;
}

static const mp_stream_p_t pb_bluetooth_stdio_obj_stream_p = {
    .read = pb_bluetooth_stdio_read,
    .write = pb_bluetooth_stdio_write,
    .ioctl = pb_bluetooth_stdio_ioctl,
    .is_text = false,
};

static MP_DEFINE_CONST_OBJ_TYPE(
    pb_bluetooth_stdio_obj_type,
    MP_QSTR_BluetoothStdio,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    protocol, &pb_bluetooth_stdio_obj_stream_p,
    locals_dict, &pb_stdio_locals_dict);

static const pb_stdio_obj_t pb_bluetooth_stdio_obj = {
    .base = { .type = &pb_bluetooth_stdio_obj_type }
};

const pb_text_io_wrapper_t pb_bluetooth_stdio_wrapper_obj = {
    .base = { .type = &pb_text_io_wrapper_obj_type },
    .binary_stream_obj = MP_OBJ_FROM_PTR(&pb_bluetooth_stdio_obj),
    .binary_stream_p = &pb_bluetooth_stdio_obj_stream_p,
};

#endif // PYBRICKS_PY_STDIO && PBSYS_CONFIG_BLUETOOTH
