// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include "py/mpconfig.h"

#if PYBRICKS_PY_EXPERIMENTAL

#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"

#include <pybricks/experimental.h>
#include <pybricks/robotics.h>

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

#include <pbio/config.h>

#if PBIO_CONFIG_ENABLE_SYS && PYBRICKS_PY_PARAMETERS_BUTTON

#include <pybricks/parameters.h>
#include <pbsys/user_program.h>

STATIC mp_obj_t mod_experimental_set_stop_button(mp_obj_t buttons_in) {
    pbio_button_flags_t buttons = 0;

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // try an iterator first in case there are multiple buttons
        mp_obj_t iter = mp_getiter(buttons_in, NULL);
        mp_obj_t item;
        while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
            buttons |= pb_type_enum_get_value(item, &pb_enum_type_Button);
        }
        nlr_pop();
    } else {
        // mp_getiter() will raise an exception if it is not an iter, so we
        // will end up here in that case, where we are expecting a single
        // button enum value. Technically there could be other error that
        // get us here, but they should be rare and will likely be a ValueError
        // which is the same error that will be raised here.
        buttons = pb_type_enum_get_value(buttons_in, &pb_enum_type_Button);
    }

    pbsys_user_program_set_stop_buttons(buttons);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_experimental_set_stop_button_obj, mod_experimental_set_stop_button);

#endif

STATIC const mp_rom_map_elem_t experimental_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_experimental_c) },
    #if PYBRICKS_HUB_EV3BRICK
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mod_experimental___init___obj) },
    { MP_ROM_QSTR(MP_QSTR_pthread_raise), MP_ROM_PTR(&mod_experimental_pthread_raise_obj) },
    #endif // PYBRICKS_HUB_EV3BRICK
    #if PBIO_CONFIG_ENABLE_SYS && PYBRICKS_PY_PARAMETERS_BUTTON
    { MP_ROM_QSTR(MP_QSTR_set_stop_button), MP_ROM_PTR(&mod_experimental_set_stop_button_obj) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(pb_module_experimental_globals, experimental_globals_table);

const mp_obj_module_t pb_module_experimental = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&pb_module_experimental_globals,
};

#endif // PYBRICKS_PY_EXPERIMENTAL
