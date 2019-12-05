// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include <pbio/sound.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"
#include "py/mphal.h"

#include "modbuiltins.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"


// pybricks.builtins.Speaker class object structure
typedef struct _builtins_Speaker_obj_t {
    mp_obj_base_t base;
    uint8_t foo; // DELETEME
    pbio_sound_t *sound;
} builtins_Speaker_obj_t;

// pybricks.builtins.Speaker.__init__/__new__
mp_obj_t builtins_Speaker_obj_make_new(uint8_t volume) {

    builtins_Speaker_obj_t *self = m_new_obj(builtins_Speaker_obj_t);
    self->base.type = &builtins_Speaker_type;

    // Init/reset speaker
    pb_assert(pbio_sound_get(&self->sound));
    pb_assert(pbio_sound_set_volume(self->sound, 20));
    
    return self;
}

// pybricks.builtins.Speaker.volume
STATIC mp_obj_t builtins_Speaker_volume(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(volume)
    );
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    mp_int_t vol_arg = pb_obj_get_int(volume);
    vol_arg = vol_arg > 100 ? 100 : vol_arg;
    vol_arg = vol_arg < 0   ? 0   : vol_arg;
    
    pb_assert(pbio_sound_set_volume(self->sound, vol_arg));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Speaker_volume_obj, 0, builtins_Speaker_volume);

// pybricks.builtins.Speaker.busy
STATIC mp_obj_t builtins_Speaker_busy(mp_obj_t self_in) {
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // DELETEME
    bool busy = self->foo % 2;

    return mp_obj_new_bool(busy);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(builtins_Speaker_busy_obj, builtins_Speaker_busy);

// pybricks.builtins.Speaker.stop
STATIC mp_obj_t builtins_Speaker_stop(mp_obj_t self_in) {
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // DELETEME
    self->foo += 1;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(builtins_Speaker_stop_obj, builtins_Speaker_stop);

// pybricks.builtins.Speaker.wait
STATIC mp_obj_t builtins_Speaker_wait(mp_obj_t self_in) {
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // DELETEME
    bool busy = self->foo * 0;

    // Wait for a background sound to finish
    while (busy) {
        mp_hal_delay_ms(10);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(builtins_Speaker_wait_obj, builtins_Speaker_wait);

// pybricks.builtins.Speaker.say
STATIC mp_obj_t builtins_Speaker_say(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(phrase),
        PB_ARG_DEFAULT_TRUE(wait)
    );
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    const char *text = mp_obj_str_get_str(phrase);
    bool blocking = mp_obj_is_true(wait);

    // DELETEME
    printf("The user wants to say at volume %d (%s):\n%s\n",
           self->foo,
           blocking ? "blocking" : "background",
           text);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Speaker_say_obj, 0, builtins_Speaker_say);

// pybricks.builtins.Speaker.beep
STATIC mp_obj_t builtins_Speaker_beep(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_INT(frequency, 500),
        PB_ARG_DEFAULT_INT(duration, 100),
        PB_ARG_DEFAULT_TRUE(wait)
    );
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    bool blocking = mp_obj_is_true(wait);

    if (!blocking) {
        pb_assert(PBIO_ERROR_NOT_IMPLEMENTED);
    }

    pbio_error_t err;
    mp_int_t freq = pb_obj_get_int(frequency);
    mp_int_t ms = pb_obj_get_int(duration);

    while ((err = pbio_sound_beep(self->sound, freq, ms)) == PBIO_ERROR_AGAIN) {
        mp_hal_delay_ms(10);
    }
    pb_assert(err);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Speaker_beep_obj, 0, builtins_Speaker_beep);

// pybricks.builtins.Speaker.play
STATIC mp_obj_t builtins_Speaker_play(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(file),
        PB_ARG_DEFAULT_TRUE(wait)
    );
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    const char *path = mp_obj_str_get_str(file);
    bool blocking = mp_obj_is_true(wait);

    // DELETEME
    printf("Play this file at volume %d (%s):\n%s\n",
           self->foo,
           blocking ? "blocking" : "background",
           path
    );

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Speaker_play_obj, 0, builtins_Speaker_play);

// dir(pybricks.builtins.Speaker)
STATIC const mp_rom_map_elem_t builtins_Speaker_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_busy   ), MP_ROM_PTR(&builtins_Speaker_busy_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait   ), MP_ROM_PTR(&builtins_Speaker_wait_obj) },
    { MP_ROM_QSTR(MP_QSTR_beep   ), MP_ROM_PTR(&builtins_Speaker_beep_obj) },
    { MP_ROM_QSTR(MP_QSTR_play   ), MP_ROM_PTR(&builtins_Speaker_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_say    ), MP_ROM_PTR(&builtins_Speaker_say_obj ) },
    { MP_ROM_QSTR(MP_QSTR_stop   ), MP_ROM_PTR(&builtins_Speaker_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_volume ), MP_ROM_PTR(&builtins_Speaker_volume_obj) },
};
STATIC MP_DEFINE_CONST_DICT(builtins_Speaker_locals_dict, builtins_Speaker_locals_dict_table);

// type(pybricks.builtins.Speaker)
const mp_obj_type_t builtins_Speaker_type = {
    { &mp_type_type },
    .name = MP_QSTR_Speaker,
    .locals_dict = (mp_obj_dict_t*)&builtins_Speaker_locals_dict,
};

