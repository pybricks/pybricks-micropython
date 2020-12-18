// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Speaker class for playing sounds.

// TODO: functions implementations need to be moved to lib/pbio/src/sound/sound.c
// so that they can be used by pbsys as well.

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_SPEAKER

#include <pbdrv/sound.h>

#include "py/mphal.h"
#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct {
    mp_obj_base_t base;
    bool intialized;
} pb_type_Speaker_obj_t;

STATIC pb_type_Speaker_obj_t pb_type_Speaker_singleton;

STATIC uint16_t waveform_data[128];

STATIC mp_obj_t pb_type_Speaker_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    pb_type_Speaker_obj_t *self = &pb_type_Speaker_singleton;
    if (!self->intialized) {
        self->base.type = &pb_type_Speaker;
        self->intialized = true;
    }
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t pb_type_Speaker_beep(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Speaker_obj_t, self,
        PB_ARG_DEFAULT_INT(frequency, 500),
        PB_ARG_DEFAULT_INT(duration, 100));

    (void)self; // unused
    mp_int_t frequency = pb_obj_get_int(frequency_in);
    mp_int_t duration = pb_obj_get_int(duration_in);

    // TODO: allow other wave shapes - sine, triangle, sawtooth
    // TODO: apply scaling for volume level
    // TODO: don't recreate waveform if it hasn't changed shape or volume

    // create square wave
    int i = 0;
    for (; i < MP_ARRAY_SIZE(waveform_data) / 2; i++) {
        waveform_data[i] = 0;
    }
    for (; i < MP_ARRAY_SIZE(waveform_data); i++) {
        waveform_data[i] = UINT16_MAX;
    }

    pbdrv_sound_start(&waveform_data[0], MP_ARRAY_SIZE(waveform_data), frequency * MP_ARRAY_SIZE(waveform_data));

    if (duration < 0) {
        return mp_const_none;
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_hal_delay_ms(duration);
        pbdrv_sound_stop();
        nlr_pop();
    } else {
        pbdrv_sound_stop();
        nlr_jump(nlr.ret_val);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Speaker_beep_obj, 1, pb_type_Speaker_beep);

STATIC const mp_rom_map_elem_t pb_type_Speaker_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_beep), MP_ROM_PTR(&pb_type_Speaker_beep_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_Speaker_locals_dict, pb_type_Speaker_locals_dict_table);

const mp_obj_type_t pb_type_Speaker = {
    { &mp_type_type },
    .name = MP_QSTR_Speaker,
    .make_new = pb_type_Speaker_make_new,
    .locals_dict = (mp_obj_dict_t *)&pb_type_Speaker_locals_dict,
};

#endif // PYBRICKS_PY_COMMON_SPEAKER
