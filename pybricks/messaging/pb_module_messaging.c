// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_MESSAGING

#include <stdio.h>
#include <string.h>

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/stream.h"

#include <pbdrv/bluetooth.h>

#include <pbio/int_math.h>
#include <pbio/util.h>

#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>

#include <pybricks/util_pb/pb_error.h>
#include <pybricks/tools/pb_type_async.h>

#define DEBUG 0

#if DEBUG
#include <pbio/debug.h>
#define DEBUG_PRINT pbio_debug
#else
#define DEBUG_PRINT(...)
#endif

typedef struct
{
    mp_obj_base_t base;
    uint32_t num_results;
    uint32_t num_results_max;
    pbdrv_bluetooth_inquiry_result_t results[];
} pb_messaging_bluetooth_scan_result_obj_t;

static mp_obj_t pb_messaging_bluetooth_scan_close(mp_obj_t self_in) {
    pb_messaging_bluetooth_scan_result_obj_t *self = MP_OBJ_TO_PTR(self_in);
    DEBUG_PRINT("rfcomm scan data freed\n");
    self->num_results_max = 0;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pb_messaging_bluetooth_scan_close_obj, pb_messaging_bluetooth_scan_close);

static const mp_rom_map_elem_t pb_messaging_bluetooth_scan_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&pb_messaging_bluetooth_scan_close_obj) },
};
static MP_DEFINE_CONST_DICT(pb_messaging_bluetooth_scan_locals_dict, pb_messaging_bluetooth_scan_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_messaging_bluetooth_scan, MP_QSTR_bluetooth_scan, MP_TYPE_FLAG_NONE, locals_dict, &pb_messaging_bluetooth_scan_locals_dict);

/**
 * Utility for converting a bluetooth address byte buffer to a string.
 *
 * The result does not persist. Intended for instant consumption.
 *
 * @param  [in]  address 6-byte bluetooth address.
 * @return               Formatted bluetooth address string.
 */
static char *format_bluetooth_address(uint8_t *address) {
    static char bdaddr_str[18];
    snprintf(bdaddr_str, sizeof(bdaddr_str), "%02X:%02X:%02X:%02X:%02X:%02X",
        address[0], address[1], address[2], address[3], address[4], address[5]);
    return bdaddr_str;
}

/**
 * Maps the inquiry results to a list of dictionary, to be returned to the user.
 */
static mp_obj_t pb_messaging_bluetooth_scan_return_map(mp_obj_t parent_obj) {

    pb_messaging_bluetooth_scan_result_obj_t *scanner = MP_OBJ_TO_PTR(parent_obj);

    mp_obj_t list = mp_obj_new_list(0, NULL);

    for (uint32_t i = 0; i < scanner->num_results; i++) {
        mp_obj_t dict = mp_obj_new_dict(0);
        pbdrv_bluetooth_inquiry_result_t *result = &scanner->results[i];
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_address), mp_obj_new_str(format_bluetooth_address(result->bdaddr), 17));
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_name), mp_obj_new_str(result->name, strlen(result->name)));
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_rssi), mp_obj_new_int(result->rssi));
        mp_obj_dict_store(dict, MP_ROM_QSTR(MP_QSTR_class), mp_obj_new_int(result->class_of_device));
        mp_obj_list_append(list, dict);
    }
    return list;
}

// pybricks.messaging.bluetooth_scan
static mp_obj_t pb_messaging_bluetooth_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_INT(timeout, 10000),
        PB_ARG_DEFAULT_INT(num_results, 5));

    // Allocate the maximum number of expected results.
    uint32_t num_results_max = mp_obj_get_int(num_results_in);
    if (!num_results_max) {
        num_results_max = 1;
    }
    pb_messaging_bluetooth_scan_result_obj_t *scanner = mp_obj_malloc_var_with_finaliser(
        pb_messaging_bluetooth_scan_result_obj_t, pbdrv_bluetooth_inquiry_result_t,
        num_results_max, &pb_type_messaging_bluetooth_scan);

    // Initialize at zero results.
    scanner->num_results = 0;
    scanner->num_results_max = mp_obj_get_int(num_results_in);
    pb_assert(pbdrv_bluetooth_start_inquiry_scan(scanner->results, &scanner->num_results, &scanner->num_results_max, mp_obj_get_int(timeout_in)));

    // Create an awaitable with a reference to our result to keep it from being
    // garbage collected.
    pb_type_async_t *iter = NULL;
    pb_type_async_t config = {
        .iter_once = pbdrv_bluetooth_await_classic_task,
        .parent_obj = MP_OBJ_FROM_PTR(scanner),
        .return_map = pb_messaging_bluetooth_scan_return_map,
    };
    return pb_type_async_wait_or_await(&config, &iter, false);
}
// See also messaging_globals_table below. This function object is added there to make it importable.
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_messaging_bluetooth_scan_obj, 0, pb_messaging_bluetooth_scan);

// pybricks.messaging.local_address
static mp_obj_t pb_messaging_local_address(void) {
    bdaddr_t address;
    pbdrv_bluetooth_local_address(address);
    return mp_obj_new_str(format_bluetooth_address(address), 17);
}
static MP_DEFINE_CONST_FUN_OBJ_0(pb_messaging_local_address_obj, pb_messaging_local_address);

// RFCOMM Socket implementation

typedef struct {
    mp_obj_base_t base;
    pbdrv_bluetooth_rfcomm_conn_t conn;
} pb_type_messaging_rfcomm_socket_obj_t;

static mp_uint_t pb_type_messaging_rfcomm_socket_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    pb_type_messaging_rfcomm_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t bytes_received;
    pbio_error_t err = pbdrv_bluetooth_rfcomm_recv(&self->conn, buf, size, &bytes_received);
    if (err != PBIO_SUCCESS) {
        *errcode = MP_EIO;
        return MP_STREAM_ERROR;
    }
    return bytes_received;
}

static mp_uint_t pb_type_messaging_rfcomm_socket_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    pb_type_messaging_rfcomm_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t bytes_sent;
    pbio_error_t err = pbdrv_bluetooth_rfcomm_send(&self->conn, buf, size, &bytes_sent);
    if (err != PBIO_SUCCESS) {
        *errcode = MP_EIO;
        return MP_STREAM_ERROR;
    }
    return bytes_sent;
}

static mp_uint_t pb_type_messaging_rfcomm_socket_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    pb_type_messaging_rfcomm_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    switch (request) {
        case MP_STREAM_POLL: {
            mp_uint_t flags = 0;
            if ((request & MP_STREAM_POLL_HUP) && !pbdrv_bluetooth_rfcomm_is_connected(&self->conn)) {
                flags |= MP_STREAM_POLL_HUP;
            }
            if ((request & MP_STREAM_POLL_RD) && pbdrv_bluetooth_rfcomm_is_readable(&self->conn)) {
                flags |= MP_STREAM_POLL_RD;
            }
            if ((request & MP_STREAM_POLL_WR) && pbdrv_bluetooth_rfcomm_is_writeable(&self->conn)) {
                flags |= MP_STREAM_POLL_WR;
            }
            return flags;
        }
        case MP_STREAM_CLOSE: {
            pbdrv_bluetooth_rfcomm_close(&self->conn);
            return 0;
        }
        case MP_STREAM_FLUSH: {
            // No buffering, so nothing to flush.
            return 0;
        }
        default:
            return MP_STREAM_ERROR;
    }
}

static const mp_stream_p_t pb_type_messaging_rfcomm_socket_stream_p = {
    .read = pb_type_messaging_rfcomm_socket_read,
    .write = pb_type_messaging_rfcomm_socket_write,
    .ioctl = pb_type_messaging_rfcomm_socket_ioctl,
};

static void pb_type_messaging_rfcomm_socket_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pb_type_messaging_rfcomm_socket_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "RFCOMMSocket(conn_id=%d)", self->conn.conn_id);
}

// Forward declaration of the RFCOMMSocket type
static const mp_obj_type_t pb_type_messaging_rfcomm_socket;

// Constructor for RFCOMMSocket
static mp_obj_t pb_type_messaging_rfcomm_socket_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    pb_type_messaging_rfcomm_socket_obj_t *self = mp_obj_malloc(pb_type_messaging_rfcomm_socket_obj_t, &pb_type_messaging_rfcomm_socket);
    self->conn.conn_id = -1;
    return MP_OBJ_FROM_PTR(self);
}

// RFCOMM connect method

typedef struct {
    pb_type_messaging_rfcomm_socket_obj_t *socket;
    bdaddr_t bdaddr;
    int32_t timeout;
} pb_type_messaging_rfcomm_socket_connect_context_t;

static pbio_error_t pb_type_messaging_rfcomm_socket_connect_iterate(pbio_os_state_t *state, mp_obj_t parent_obj) {
    pb_type_messaging_rfcomm_socket_connect_context_t *context = (pb_type_messaging_rfcomm_socket_connect_context_t *)parent_obj;
    return pbdrv_bluetooth_rfcomm_connect(state, context->bdaddr, context->timeout, &context->socket->conn);
}

static mp_obj_t pb_type_messaging_rfcomm_socket_connect_return_map(mp_obj_t context_obj) {
    pb_type_messaging_rfcomm_socket_connect_context_t *context = (pb_type_messaging_rfcomm_socket_connect_context_t *)context_obj;
    return MP_OBJ_FROM_PTR(context->socket);
}

static mp_obj_t pb_type_messaging_rfcomm_socket_connect(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args, pb_type_messaging_rfcomm_socket_obj_t, self, PB_ARG_REQUIRED(address), PB_ARG_DEFAULT_INT(timeout, 10000));

    // Check if socket is already connected
    if (pbdrv_bluetooth_rfcomm_is_connected(&self->conn)) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("socket is already connected"));
    }

    pb_type_messaging_rfcomm_socket_connect_context_t *context = m_new(pb_type_messaging_rfcomm_socket_connect_context_t, 1);
    context->socket = self;

    const char *address_str = mp_obj_str_get_str(address_in);
    if (!pbdrv_bluetooth_str_to_bdaddr(address_str, context->bdaddr)) {
        m_del(pb_type_messaging_rfcomm_socket_connect_context_t, context, 1);
        mp_raise_ValueError(MP_ERROR_TEXT("invalid Bluetooth address format"));
    }

    context->timeout = mp_obj_get_int(timeout_in);
    if (context->timeout < 0) {
        m_del(pb_type_messaging_rfcomm_socket_connect_context_t, context, 1);
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be non-negative"));
    }

    pb_type_async_t *awaitable_ptr = NULL;
    pb_type_async_t config = {
        .iter_once = pb_type_messaging_rfcomm_socket_connect_iterate,
        .parent_obj = MP_OBJ_FROM_PTR(context),
        .return_map = pb_type_messaging_rfcomm_socket_connect_return_map,
    };
    return pb_type_async_wait_or_await(&config, &awaitable_ptr, true);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_messaging_rfcomm_socket_connect_obj, 1, pb_type_messaging_rfcomm_socket_connect);

// RFCOMM listen method

typedef struct {
    pb_type_messaging_rfcomm_socket_obj_t *socket;
    int32_t timeout;
} pb_type_messaging_rfcomm_socket_listen_context_t;

static pbio_error_t pb_type_messaging_rfcomm_socket_listen_iterate(pbio_os_state_t *state, mp_obj_t parent_obj) {
    pb_type_messaging_rfcomm_socket_listen_context_t *context = (pb_type_messaging_rfcomm_socket_listen_context_t *)parent_obj;
    return pbdrv_bluetooth_rfcomm_listen(state, context->timeout, &context->socket->conn);
}

static mp_obj_t pb_type_messaging_rfcomm_socket_listen_return_map(mp_obj_t context_obj) {
    pb_type_messaging_rfcomm_socket_listen_context_t *context = (pb_type_messaging_rfcomm_socket_listen_context_t *)context_obj;
    return MP_OBJ_FROM_PTR(context->socket);
}

static mp_obj_t pb_type_messaging_rfcomm_socket_listen(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args, pb_type_messaging_rfcomm_socket_obj_t, self, PB_ARG_DEFAULT_INT(timeout, 0));

    // Check if socket is already connected
    if (pbdrv_bluetooth_rfcomm_is_connected(&self->conn)) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("socket is already connected"));
    }

    pb_type_messaging_rfcomm_socket_listen_context_t *context = m_new(pb_type_messaging_rfcomm_socket_listen_context_t, 1);
    context->socket = self;

    context->timeout = mp_obj_get_int(timeout_in);
    if (context->timeout < 0) {
        m_del(pb_type_messaging_rfcomm_socket_listen_context_t, context, 1);
        mp_raise_ValueError(MP_ERROR_TEXT("timeout must be non-negative"));
    }

    pb_type_async_t *awaitable_ptr = NULL;
    pb_type_async_t config = {
        .iter_once = pb_type_messaging_rfcomm_socket_listen_iterate,
        .parent_obj = MP_OBJ_FROM_PTR(context),
        .return_map = pb_type_messaging_rfcomm_socket_listen_return_map,
    };
    return pb_type_async_wait_or_await(&config, &awaitable_ptr, true);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_messaging_rfcomm_socket_listen_obj, 0, pb_type_messaging_rfcomm_socket_listen);

// Context manager support for RFCOMMSocket
static mp_obj_t pb_type_messaging_rfcomm_socket_enter(mp_obj_t self_in) {
    return self_in;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pb_type_messaging_rfcomm_socket_enter_obj, pb_type_messaging_rfcomm_socket_enter);

static mp_obj_t pb_type_messaging_rfcomm_socket_exit(size_t n_args, const mp_obj_t *args) {
    // args[0] is self, args[1:4] are exc_type, exc_val, exc_tb
    pb_type_messaging_rfcomm_socket_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    pbdrv_bluetooth_rfcomm_close(&self->conn);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR(pb_type_messaging_rfcomm_socket_exit_obj, 4, pb_type_messaging_rfcomm_socket_exit);

static const mp_rom_map_elem_t pb_type_messaging_rfcomm_socket_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&pb_type_messaging_rfcomm_socket_enter_obj)},
    {MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&pb_type_messaging_rfcomm_socket_exit_obj)},
    {MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&pb_type_messaging_rfcomm_socket_connect_obj)},
    {MP_ROM_QSTR(MP_QSTR_listen), MP_ROM_PTR(&pb_type_messaging_rfcomm_socket_listen_obj)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj)},
};
static MP_DEFINE_CONST_DICT(pb_type_messaging_rfcomm_socket_locals_dict, pb_type_messaging_rfcomm_socket_locals_dict_table);

static MP_DEFINE_CONST_OBJ_TYPE(
    pb_type_messaging_rfcomm_socket,
    MP_QSTR_RFCOMMSocket,
    MP_TYPE_FLAG_NONE,
    make_new,
    pb_type_messaging_rfcomm_socket_make_new,
    print,
    pb_type_messaging_rfcomm_socket_print,
    protocol,
    &pb_type_messaging_rfcomm_socket_stream_p,
    locals_dict,
    &pb_type_messaging_rfcomm_socket_locals_dict);

static const mp_rom_map_elem_t messaging_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_messaging)},
    {MP_ROM_QSTR(MP_QSTR_local_address), MP_ROM_PTR(&pb_messaging_local_address_obj)},
    {MP_ROM_QSTR(MP_QSTR_bluetooth_scan), MP_ROM_PTR(&pb_messaging_bluetooth_scan_obj)},
    {MP_ROM_QSTR(MP_QSTR_RFCOMMSocket), MP_ROM_PTR(&pb_type_messaging_rfcomm_socket)},
};
static MP_DEFINE_CONST_DICT(pb_module_messaging_globals, messaging_globals_table);

const mp_obj_module_t pb_module_messaging = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_messaging_globals,
};

#if !MICROPY_MODULE_BUILTIN_SUBPACKAGES
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_messaging, pb_module_messaging);
#endif

#endif // PYBRICKS_PY_MESSAGING
