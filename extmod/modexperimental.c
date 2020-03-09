// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// C implementation of Pybricks experimental module.
// Also see pybricks/experimental.py.

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#if !MICROPY_MODULE_BUILTIN_INIT
#error "pybricks.experimental module requires that MICROPY_MODULE_BUILTIN_INIT is enabled"
#endif

#if PYBRICKS_HUB_EV3
#include <signal.h>
#endif // PYBRICKS_HUB_EV3

#include "py/mpthread.h"
#include "py/obj.h"
#include "py/runtime.h"

#if PYBRICKS_HUB_EV3
STATIC void sighandler() {
    // we just want the signal to interrupt system calls
}
#endif // PYBRICKS_HUB_EV3

STATIC mp_obj_t mod_experimental___init__() {
    #if PYBRICKS_HUB_EV3
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);
    #endif // PYBRICKS_HUB_EV3

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_experimental___init___obj, mod_experimental___init__);

STATIC mp_obj_t mod_experimental_pthread_raise(mp_obj_t thread_id_in, mp_obj_t ex_in) {
    mp_uint_t thread_id = mp_obj_int_get_truncated(thread_id_in);
    if (ex_in != mp_const_none && !mp_obj_is_exception_instance(ex_in)) {
        mp_raise_TypeError("must be an exception or None");
    }
    return mp_obj_new_int(mp_thread_schedule_exception(thread_id, ex_in));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_experimental_pthread_raise_obj, mod_experimental_pthread_raise);

STATIC const mp_rom_map_elem_t mod_experimental_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental_c) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_experimental___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_pthread_raise), MP_ROM_PTR(&mod_experimental_pthread_raise_obj) },
};
STATIC MP_DEFINE_CONST_DICT(mod_experimental_globals, mod_experimental_globals_table);

const mp_obj_module_t pb_module_experimental = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mod_experimental_globals,
};

#endif // PYBRICKS_PY_EXPERIMENTAL
