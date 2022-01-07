// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

// Speaker class for playing sounds.

// TODO: functions implementations need to be moved to lib/pbio/src/sound/sound.c
// so that they can be used by pbsys as well.

// TODO: share code with ev3dev Speaker type

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
    bool initialized;
} pb_type_Speaker_obj_t;

STATIC pb_type_Speaker_obj_t pb_type_Speaker_singleton;

STATIC uint16_t waveform_data[128];

STATIC void pb_type_Speaker_start_beep(uint32_t frequency) {
    // TODO: allow other wave shapes - sine, triangle, sawtooth
    // TODO: apply scaling for volume level
    // TODO: don't recreate waveform if it hasn't changed shape or volume

    if (frequency == 0) {
        // 0 frequency is just a flat line
        for (int i = 0; i < MP_ARRAY_SIZE(waveform_data); i++) {
            waveform_data[i] = INT16_MAX;
        }
    } else {
        // create square wave
        int i = 0;
        for (; i < MP_ARRAY_SIZE(waveform_data) / 2; i++) {
            waveform_data[i] = 0;
        }
        for (; i < MP_ARRAY_SIZE(waveform_data); i++) {
            waveform_data[i] = UINT16_MAX;
        }
    }

    if (frequency < 64) {
        frequency = 64;
    }
    if (frequency > 24000) {
        frequency = 24000;
    }

    pbdrv_sound_start(&waveform_data[0], MP_ARRAY_SIZE(waveform_data), frequency * MP_ARRAY_SIZE(waveform_data));
}

STATIC void pb_type_Speaker_stop_beep(void) {
    pbdrv_sound_stop();
}

STATIC mp_obj_t pb_type_Speaker_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    pb_type_Speaker_obj_t *self = &pb_type_Speaker_singleton;
    if (!self->initialized) {
        self->base.type = &pb_type_Speaker;
        self->initialized = true;
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

    pb_type_Speaker_start_beep(frequency);

    if (duration < 0) {
        return mp_const_none;
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_hal_delay_ms(duration);
        pb_type_Speaker_stop_beep();
        nlr_pop();
    } else {
        pb_type_Speaker_stop_beep();
        nlr_jump(nlr.ret_val);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Speaker_beep_obj, 1, pb_type_Speaker_beep);

STATIC void pb_type_Speaker_play_note(pb_type_Speaker_obj_t *self, mp_obj_t obj, int duration) {
    const char *note = mp_obj_str_get_str(obj);
    int pos = 0;
    mp_float_t freq;
    bool release = true;

    // Note names can be A-G followed by optional # (sharp) or b (flat) or R for rest
    switch (note[pos++]) {
        case 'C':
            switch (note[pos++]) {
                case 'b':
                    mp_raise_ValueError(MP_ERROR_TEXT("Cb is not allowed"));
                    break;
                case '#':
                    freq = MICROPY_FLOAT_CONST(17.32);
                    break;
                default:
                    pos--;
                    freq = MICROPY_FLOAT_CONST(16.35);
                    break;
            }
            break;
        case 'D':
            switch (note[pos++]) {
                case 'b':
                    freq = MICROPY_FLOAT_CONST(17.32);
                    break;
                case '#':
                    freq = MICROPY_FLOAT_CONST(19.45);
                    break;
                default:
                    pos--;
                    freq = MICROPY_FLOAT_CONST(18.35);
                    break;
            }
            break;
        case 'E':
            switch (note[pos++]) {
                case 'b':
                    freq = MICROPY_FLOAT_CONST(19.45);
                    break;
                case '#':
                    mp_raise_ValueError(MP_ERROR_TEXT("E# is not allowed"));
                    break;
                default:
                    pos--;
                    freq = MICROPY_FLOAT_CONST(20.60);
                    break;
            }
            break;
        case 'F':
            switch (note[pos++]) {
                case 'b':
                    mp_raise_ValueError(MP_ERROR_TEXT("Fb is not allowed"));
                    break;
                case '#':
                    freq = MICROPY_FLOAT_CONST(23.12);
                    break;
                default:
                    pos--;
                    freq = MICROPY_FLOAT_CONST(21.83);
                    break;
            }
            break;
        case 'G':
            switch (note[pos++]) {
                case 'b':
                    freq = MICROPY_FLOAT_CONST(23.12);
                    break;
                case '#':
                    freq = MICROPY_FLOAT_CONST(25.96);
                    break;
                default:
                    pos--;
                    freq = MICROPY_FLOAT_CONST(24.50);
                    break;
            }
            break;
        case 'A':
            switch (note[pos++]) {
                case 'b':
                    freq = MICROPY_FLOAT_CONST(25.96);
                    break;
                case '#':
                    freq = MICROPY_FLOAT_CONST(29.14);
                    break;
                default:
                    pos--;
                    freq = MICROPY_FLOAT_CONST(27.50);
                    break;
            }
            break;
        case 'B':
            switch (note[pos++]) {
                case 'b':
                    freq = MICROPY_FLOAT_CONST(29.14);
                    break;
                case '#':
                    mp_raise_ValueError(MP_ERROR_TEXT("B# is not allowed"));
                    break;
                default:
                    pos--;
                    freq = MICROPY_FLOAT_CONST(30.87);
                    break;
            }
            break;
        case 'R':
            freq = MICROPY_FLOAT_CONST(0.0);
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("Missing note name A-G or R"));
            break;
    }

    // Note name must be followed by the octave number
    if (freq != 0) {
        int octave = note[pos++] - '0';
        if (octave < 2 || octave > 8) {
            mp_raise_ValueError(MP_ERROR_TEXT("Missing octave number 2-8"));
        }
        freq *= 2 << (octave - 1);
    }

    // '/' delimiter is required between octave and fraction
    if (note[pos++] != '/') {
        mp_raise_ValueError(MP_ERROR_TEXT("Missing /"));
    }

    // The fractional size of the note, e.g. 4 = quarter note
    int fraction = note[pos++] - '0';
    if (fraction < 0 || fraction > 9) {
        mp_raise_ValueError(MP_ERROR_TEXT("Missing fractional value 1, 2, 4, 8, etc."));
    }

    // optional second digit
    int fraction2 = note[pos++] - '0';
    if (fraction2 < 0 || fraction2 > 9) {
        pos--;
    } else {
        fraction = fraction * 10 + fraction2;
    }

    duration /= fraction;

    // optional decorations

    if (note[pos++] == '.') {
        // dotted note has length extended by 1/2
        duration = 3 * duration / 2;
    } else {
        pos--;
    }

    if (note[pos++] == '_') {
        // note with tie/slur is not released
        release = false;
    } else {
        pos--;
    }

    pb_type_Speaker_start_beep((uint32_t)freq);

    // Normally, we want there to be a period of no sound (release) so that
    // notes are distinct instead of running together. To sound good, the
    // release period is made proportional to duration of the note.
    if (release) {
        mp_hal_delay_ms(7 * duration / 8);
        pb_type_Speaker_stop_beep();
        mp_hal_delay_ms(duration / 8);
    } else {
        mp_hal_delay_ms(duration);
    }
}

STATIC mp_obj_t pb_type_Speaker_play_notes(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Speaker_obj_t, self,
        PB_ARG_REQUIRED(notes),
        PB_ARG_DEFAULT_INT(tempo, 120));

    // length of whole note in milliseconds = 4 quarter/whole * 60 s/min * 1000 ms/s / tempo quarter/min
    int duration = 4 * 60 * 1000 / pb_obj_get_int(tempo_in);

    nlr_buf_t nlr;
    mp_obj_t item;
    mp_obj_t iterable = mp_getiter(notes_in, NULL);
    if (nlr_push(&nlr) == 0) {
        while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
            pb_type_Speaker_play_note(self, item, duration);
        }
        // in case the last note has '_'
        pb_type_Speaker_stop_beep();
        nlr_pop();
    } else {
        // ensure that sound stops if an exception is raised
        pb_type_Speaker_stop_beep();
        nlr_jump(nlr.ret_val);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Speaker_play_notes_obj, 1, pb_type_Speaker_play_notes);

STATIC const mp_rom_map_elem_t pb_type_Speaker_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_beep), MP_ROM_PTR(&pb_type_Speaker_beep_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_notes), MP_ROM_PTR(&pb_type_Speaker_play_notes_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pb_type_Speaker_locals_dict, pb_type_Speaker_locals_dict_table);

const mp_obj_type_t pb_type_Speaker = {
    { &mp_type_type },
    .name = MP_QSTR_Speaker,
    .make_new = pb_type_Speaker_make_new,
    .locals_dict = (mp_obj_dict_t *)&pb_type_Speaker_locals_dict,
};

#endif // PYBRICKS_PY_COMMON_SPEAKER
