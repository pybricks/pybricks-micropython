// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "modbuiltins.h"
#include "pberror.h"
#include "pbobj.h"
#include "pbkwarg.h"

// pybricks.builtins.Speaker class object structure
typedef struct _builtins_Speaker_obj_t {
    mp_obj_base_t base;
    uint8_t volume;
    uint8_t foo;
} builtins_Speaker_obj_t;

// pybricks.builtins.Speaker.__init__/__new__
mp_obj_t builtins_Speaker_obj_make_new() {
    builtins_Speaker_obj_t *self = m_new_obj(builtins_Speaker_obj_t);
    self->base.type = &builtins_Speaker_type;

    // Here we can do things like init/reset speaker
    self->volume = 50;

    return self;
}

// pybricks.builtins.Speaker.stop
STATIC mp_obj_t builtins_Speaker_stop(mp_obj_t self_in) {
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // This is an example of a method with no arguments other than self.
    // Delete this function if we don't actually want a stop method.

    self->foo += 1;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(builtins_Speaker_stop_obj, builtins_Speaker_stop);

// pybricks.builtins.Speaker.say
STATIC mp_obj_t builtins_Speaker_say(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(phrase)
    );
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    self->foo += 1;

    const char *text = mp_obj_str_get_str(phrase);

    // DELETEME
    printf("The user wants to say at volume %d:\n%s\n", self->volume, text);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Speaker_say_obj, 0, builtins_Speaker_say);

// pybricks.builtins.Speaker.beep
STATIC mp_obj_t builtins_Speaker_beep(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_DEFAULT_INT(frequency, 500),
        PB_ARG_DEFAULT_INT(duration, 100)
    );
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    // DELETEME
    printf("We should beep at %d Hz, for %d ms, at volume %d.\n",
           mp_obj_get_int(frequency),
           mp_obj_get_int(duration),
           self->volume
    );

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Speaker_beep_obj, 0, builtins_Speaker_beep);

// pybricks.builtins.Speaker.play
STATIC mp_obj_t builtins_Speaker_play(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        PB_ARG_REQUIRED(file)
    );
    builtins_Speaker_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    const char *path = mp_obj_str_get_str(file);

    // DELETEME
    printf("Play this file at volume %d:\n%s\n",
           self->volume,
           path
    );

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(builtins_Speaker_play_obj, 0, builtins_Speaker_play);

// dir(pybricks.builtins.Speaker)
STATIC const mp_rom_map_elem_t builtins_Speaker_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_beep   ), MP_ROM_PTR(&builtins_Speaker_beep_obj) },
    { MP_ROM_QSTR(MP_QSTR_play   ), MP_ROM_PTR(&builtins_Speaker_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_say    ), MP_ROM_PTR(&builtins_Speaker_say_obj ) },
    { MP_ROM_QSTR(MP_QSTR_stop   ), MP_ROM_PTR(&builtins_Speaker_stop_obj) },
};
STATIC MP_DEFINE_CONST_DICT(builtins_Speaker_locals_dict, builtins_Speaker_locals_dict_table);

// type(pybricks.builtins.Speaker)
const mp_obj_type_t builtins_Speaker_type = {
    { &mp_type_type },
    .name = MP_QSTR_Speaker,
    .locals_dict = (mp_obj_dict_t*)&builtins_Speaker_locals_dict,
};

