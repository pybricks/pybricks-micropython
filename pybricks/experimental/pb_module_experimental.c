// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2021 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#include "py/mphal.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include <pbio/util.h>

#include <pybricks/common.h>
#include <pybricks/robotics.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_pb/pb_error.h>

#if PYBRICKS_HUB_EV3BRICK
#if !MICROPY_MODULE_BUILTIN_INIT
#error "pybricks.experimental module requires that MICROPY_MODULE_BUILTIN_INIT is enabled"
#endif

#include <signal.h>

#include "py/mpthread.h"

STATIC void sighandler(int signum) {
    // we just want the signal to interrupt system calls
}

STATIC mp_obj_t mod_experimental___init__(void) {
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_experimental___init___obj, mod_experimental___init__);

STATIC mp_obj_t mod_experimental_pthread_raise(mp_obj_t thread_id_in, mp_obj_t ex_in) {
    mp_uint_t thread_id = mp_obj_int_get_truncated(thread_id_in);
    if (ex_in != mp_const_none && !mp_obj_is_exception_instance(ex_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("must be an exception or None"));
    }
    return mp_obj_new_int(mp_thread_schedule_exception(thread_id, ex_in));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_experimental_pthread_raise_obj, mod_experimental_pthread_raise);
#endif // PYBRICKS_HUB_EV3BRICK

// pybricks.experimental.hello_world
STATIC mp_obj_t experimental_hello_world(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_FUNCTION(n_args, pos_args, kw_args,
        // Add up to 8 arguments below, all separated by one comma.
        // You can choose from:
        //  PB_ARG_REQUIRED(name), This is a required argument.
        //  PB_ARG_DEFAULT_INT(name, value), Keyword argument with default int value.
        //  PB_ARG_DEFAULT_OBJ(name, value), Keyword argument with default MicroPython object.
        //  PB_ARG_DEFAULT_QSTR(name, value), Keyword argument with default string value, without quotes.
        //  PB_ARG_DEFAULT_FALSE(name), Keyword argument with default False value.
        //  PB_ARG_DEFAULT_TRUE(name), Keyword argument with default True value.
        //  PB_ARG_DEFAULT_NONE(name), Keyword argument with default None value.
        PB_ARG_REQUIRED(foo),
        PB_ARG_DEFAULT_NONE(bar));

    // This function can be used in the following ways:
    // from pybricks.experimental import hello_world
    // y = hello_world(5)
    // y = hello_world(foo=5)
    // y = hello_world(5, 6)
    // y = hello_world(foo=5, bar=6)

    // All input arguments are available as MicroPython objects with _in post-fixed to the name.
    // Use MicroPython functions or Pybricks helper functions to unpack them into C types:
    mp_int_t foo = pb_obj_get_int(foo_in);

    // You can check if an argument is none.
    if (bar_in == mp_const_none) {
        // Example of how to print.
        mp_printf(&mp_plat_print, "Bar was not given. Foo is: %d\n", foo);
    }

    // Example of returning an object. Here we return the square of the input argument.
    return mp_obj_new_int(foo * foo);
}
// See also experimental_globals_table below. This function object is added there to make it importable.
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(experimental_hello_world_obj, 0, experimental_hello_world);

#if PYBRICKS_PY_COMMON_BUFFERED_STREAM

#include <contiki.h>
#include <contiki-lib.h>
#include <lwrb/lwrb.h>

typedef struct {
    void* next;
    lwrb_t read_buf;
    lwrb_t write_buf;
    uint8_t data[0];
} experimental_echo_stream_t;

PROCESS(experimental_echo_server_process, "experimental_echo_server_process");
LIST(experimental_echo_server_streams);

PROCESS_THREAD(experimental_echo_server_process, ev, data) {
    PROCESS_BEGIN();

    for (;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

        for (experimental_echo_stream_t *stream = list_head(experimental_echo_server_streams);
            stream != NULL; stream = list_item_next(stream)) {

            // echo as much data as possible from the write stream to the read stream
            for (;;) {
                size_t available = lwrb_get_free(&stream->read_buf);

                if (available == 0) {
                    // read stream is full, try again later
                    break;
                }

                // copy at most 32 bytes at a time to avoid huge stack allocation
                uint8_t buf[32];

                if (available > sizeof(buf)) {
                    available = sizeof(buf);
                }

                available = lwrb_read(&stream->write_buf, buf, available);

                if (available == 0) {
                    // there was nothing in the write steam, try again later
                    break;
                }

                lwrb_write(&stream->read_buf, buf, available);
            }
        }
    }

    PROCESS_END();
}

STATIC mp_obj_t mod_experimental___init__(void) {
    process_start(&experimental_echo_server_process);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_experimental___init___obj, mod_experimental___init__);

STATIC void experimental_notify_echo_stream(void *context) {
    process_poll(&experimental_echo_server_process);
}

STATIC void experimental_close_echo_stream(void *context) {
    list_remove(experimental_echo_server_streams, context);
}

STATIC mp_obj_t experimental_create_echo_stream(mp_obj_t read_buf_len_in, mp_obj_t write_buf_len_in) {
    mp_int_t read_buf_len = mp_obj_get_int(read_buf_len_in);
    mp_int_t write_buf_len = mp_obj_get_int(write_buf_len_in);

    if (read_buf_len < 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("read_buf_len must be >= 1"));
    }

    if (write_buf_len < 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("write_buf_len must be >= 1"));
    }

    experimental_echo_stream_t *stream = m_new_obj_var(
        experimental_echo_stream_t, uint8_t, read_buf_len + write_buf_len + 2);

    lwrb_init(&stream->read_buf, stream->data, read_buf_len + 1);
    lwrb_init(&stream->write_buf, stream->data + read_buf_len + 1, write_buf_len + 1);

    mp_obj_t instance = pb_type_BufferedStream_obj_new(&stream->read_buf, &stream->write_buf, stream,
        experimental_notify_echo_stream, experimental_close_echo_stream);

    list_add(experimental_echo_server_streams, stream);

    return instance;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(experimental_create_echo_stream_obj, experimental_create_echo_stream);

#endif // PYBRICKS_PY_COMMON_BUFFERED_STREAM

STATIC const mp_rom_map_elem_t experimental_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental) },
    #if PYBRICKS_HUB_EV3BRICK
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_experimental___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_pthread_raise), MP_ROM_PTR(&mod_experimental_pthread_raise_obj) },
    #endif // PYBRICKS_HUB_EV3BRICK
    { MP_ROM_QSTR(MP_QSTR_hello_world), MP_ROM_PTR(&experimental_hello_world_obj) },
    #if PYBRICKS_PY_COMMON_BUFFERED_STREAM
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_experimental___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_create_echo_stream), MP_ROM_PTR(&experimental_create_echo_stream_obj) },
    #endif // PYBRICKS_PY_COMMON_BUFFERED_STREAM
};
STATIC MP_DEFINE_CONST_DICT(pb_module_experimental_globals, experimental_globals_table);

const mp_obj_module_t pb_module_experimental = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_experimental_globals,
};

#if PYBRICKS_RUNS_ON_EV3DEV
// ev3dev extends the C module in Python
MP_REGISTER_MODULE(MP_QSTR__experimental, pb_module_experimental);
#else
MP_REGISTER_MODULE(MP_QSTR_pybricks_dot_experimental, pb_module_experimental);
#endif

#endif // PYBRICKS_PY_EXPERIMENTAL
