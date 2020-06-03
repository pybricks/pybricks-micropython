// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// This is a subset of the standard CPython signal module.

#include "py/mpconfig.h"

#if PYBRICKS_PY_USIGNAL

#include <signal.h>
#include <unistd.h>

#include "py/mpthread.h"
#include "py/obj.h"
#include "py/runtime.h"

STATIC mp_obj_t usignal_pause(void) {

    MP_THREAD_GIL_EXIT();
    pause();
    MP_THREAD_GIL_ENTER();
    mp_handle_pending(true);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(usignal_pause_obj, usignal_pause);

#if MICROPY_PY_THREAD
STATIC mp_obj_t usignal_pthread_kill(mp_obj_t thread_id_in, mp_obj_t signalnum_in) {

    mp_int_t thread_id = mp_obj_int_get_truncated(thread_id_in);
    mp_int_t signalnum = mp_obj_get_int(signalnum_in);
    int err = pthread_kill(thread_id, signalnum);
    if (err) {
        mp_raise_OSError(err);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(usignal_pthread_kill_obj, usignal_pthread_kill);
#endif // MICROPY_PY_THREAD

STATIC const mp_rom_map_elem_t usignal_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_usignal) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&usignal_pause_obj) },
    #if MICROPY_PY_THREAD
    { MP_ROM_QSTR(MP_QSTR_pthread_kill), MP_ROM_PTR(&usignal_pthread_kill_obj) },
    #endif // MICROPY_PY_THREAD
    { MP_ROM_QSTR(MP_QSTR_SIGHUP), MP_ROM_INT(SIGHUP) },
    { MP_ROM_QSTR(MP_QSTR_SIGINT), MP_ROM_INT(SIGINT) },
    { MP_ROM_QSTR(MP_QSTR_SIGTERM), MP_ROM_INT(SIGTERM) },
    { MP_ROM_QSTR(MP_QSTR_SIGUSR1), MP_ROM_INT(SIGUSR1) },
    { MP_ROM_QSTR(MP_QSTR_SIGUSR2), MP_ROM_INT(SIGUSR2) },
};
STATIC MP_DEFINE_CONST_DICT(pb_module_usignal_globals, usignal_globals_table);

const mp_obj_module_t pb_module_usignal = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_usignal_globals,
};

#endif // PYBRICKS_PY_USIGNAL
