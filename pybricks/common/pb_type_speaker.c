// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2023 The Pybricks Authors

// Speaker class for playing sounds.

// TODO: functions implementations need to be moved to lib/pbio/src/sound/sound.c
// so that they can be used by pbsys as well.

// TODO: share code with ev3dev Speaker type

#include "py/mpconfig.h"

#if PYBRICKS_PY_COMMON_SPEAKER && MICROPY_PY_BUILTINS_FLOAT

#include <math.h>
#include <pbdrv/sound.h>

#include "py/mphal.h"
#include "py/obj.h"

#include <pybricks/common.h>
#include <pybricks/tools.h>
#include <pybricks/tools/pb_type_async.h>
#include <pybricks/util_mp/pb_kwarg_helper.h>
#include <pybricks/util_mp/pb_obj_helper.h>
#include <pybricks/util_pb/pb_error.h>

typedef struct {
    mp_obj_base_t base;

    // State of awaitable sound
    pb_type_async_t *iter;
    pbio_os_timer_t timer;
    mp_obj_t notes_generator;
    uint32_t note_duration;
    uint32_t scaled_duration;

    // volume in 0..100 range
    uint8_t volume;

    // The number to multiply the sample amplitude by, to attenuate the amplitude based on the defined speaker volume.
    // The original sample amplitude must be in the -1..1 range.
    uint16_t sample_attenuator;
} pb_type_Speaker_obj_t;

static mp_obj_t pb_type_Speaker_volume(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Speaker_obj_t, self,
        PB_ARG_DEFAULT_NONE(volume));

    if (volume_in == mp_const_none) {
        return MP_OBJ_NEW_SMALL_INT(self->volume);
    }

    self->volume = pb_obj_get_pct(volume_in);

    // exponential amplification (human ear perceives sample amplitude in a logarithmic way)
    self->sample_attenuator = (powf(10, self->volume / 100.0F) - 1) / 9 * INT16_MAX;

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Speaker_volume_obj, 1, pb_type_Speaker_volume);

static mp_obj_t pb_type_Speaker_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    pb_type_Speaker_obj_t *self = mp_obj_malloc(pb_type_Speaker_obj_t, type);

    // REVISIT: If a user creates two Speaker instances, this will reset the volume settings for both.
    // If done only once per singleton, however, altered volume settings would be persisted between program runs.
    self->volume = PBDRV_CONFIG_SOUND_DEFAULT_VOLUME;
    self->sample_attenuator = INT16_MAX;

    self->iter = NULL;

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t pb_type_Speaker_close(mp_obj_t self_in) {
    pbdrv_sound_stop();
    return mp_const_none;
}

static pbio_error_t pb_type_Speaker_beep_iterate_once(pbio_os_state_t *state, mp_obj_t parent_obj) {
    pb_type_Speaker_obj_t *self = MP_OBJ_TO_PTR(parent_obj);
    // The beep has already been started. We just need to await the duration
    // and then stop.
    PBIO_OS_ASYNC_BEGIN(state);
    PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&self->timer));
    pbdrv_sound_stop();
    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static mp_obj_t pb_type_Speaker_beep(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Speaker_obj_t, self,
        PB_ARG_DEFAULT_INT(frequency, 500),
        PB_ARG_DEFAULT_INT(duration, 100));

    mp_int_t frequency = pb_obj_get_int(frequency_in);
    mp_int_t duration = pb_obj_get_int(duration_in);

    pbdrv_beep_start(frequency, self->sample_attenuator);

    if (duration < 0) {
        return mp_const_none;
    }

    pbio_os_timer_set(&self->timer, pb_obj_get_int(duration_in));

    pb_type_async_t config = {
        .parent_obj = MP_OBJ_FROM_PTR(self),
        .iter_once = pb_type_Speaker_beep_iterate_once,
        .close = pb_type_Speaker_close,
    };
    // New operation always wins; ongoing sound awaitable is cancelled.
    pb_type_async_schedule_cancel(self->iter);
    return pb_type_async_wait_or_await(&config, &self->iter);

}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Speaker_beep_obj, 1, pb_type_Speaker_beep);

static void pb_type_Speaker_get_note(mp_obj_t obj, uint32_t note_ms, uint32_t *frequency, uint32_t *total_ms, uint32_t *on_ms) {
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

    *total_ms = note_ms / fraction;

    // optional decorations

    if (note[pos++] == '.') {
        // dotted note has length extended by 1/2
        *total_ms = 3 * *total_ms / 2;
    } else {
        pos--;
    }

    if (note[pos++] == '_') {
        // note with tie/slur is not released
        release = false;
    } else {
        pos--;
    }

    *frequency = (uint32_t)freq;
    *on_ms = release ? 7 * (*total_ms) / 8 : *total_ms;
}

static pbio_error_t pb_type_Speaker_play_notes_iterate_once(pbio_os_state_t *state, mp_obj_t parent_obj) {
    pb_type_Speaker_obj_t *self = MP_OBJ_TO_PTR(parent_obj);
    mp_obj_t item;

    PBIO_OS_ASYNC_BEGIN(state);

    while ((item = mp_iternext(self->notes_generator)) != MP_OBJ_STOP_ITERATION) {

        // Parse next note.
        uint32_t frequency;
        uint32_t beep_time;
        pb_type_Speaker_get_note(item, self->note_duration, &frequency, &self->scaled_duration, &beep_time);

        // On portion of the note.
        pbdrv_beep_start(frequency, self->sample_attenuator);
        pbio_os_timer_set(&self->timer, beep_time);
        PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&self->timer));

        // Off portion of the note.
        pbdrv_sound_stop();
        self->timer.duration = self->scaled_duration;
        PBIO_OS_AWAIT_UNTIL(state, pbio_os_timer_is_expired(&self->timer));
    }

    PBIO_OS_ASYNC_END(PBIO_SUCCESS);
}

static mp_obj_t pb_type_Speaker_play_notes(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    PB_PARSE_ARGS_METHOD(n_args, pos_args, kw_args,
        pb_type_Speaker_obj_t, self,
        PB_ARG_REQUIRED(notes),
        PB_ARG_DEFAULT_INT(tempo, 120));

    self->notes_generator = mp_getiter(notes_in, NULL);
    self->note_duration = 4 * 60 * 1000 / pb_obj_get_int(tempo_in);

    pb_type_async_t config = {
        .parent_obj = MP_OBJ_FROM_PTR(self),
        .iter_once = pb_type_Speaker_play_notes_iterate_once,
        .close = pb_type_Speaker_close,
    };
    // New operation always wins; ongoing sound awaitable is cancelled.
    pb_type_async_schedule_cancel(self->iter);
    return pb_type_async_wait_or_await(&config, &self->iter);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pb_type_Speaker_play_notes_obj, 1, pb_type_Speaker_play_notes);

static const mp_rom_map_elem_t pb_type_Speaker_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_volume), MP_ROM_PTR(&pb_type_Speaker_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_beep), MP_ROM_PTR(&pb_type_Speaker_beep_obj) },
    { MP_ROM_QSTR(MP_QSTR_play_notes), MP_ROM_PTR(&pb_type_Speaker_play_notes_obj) },
};
static MP_DEFINE_CONST_DICT(pb_type_Speaker_locals_dict, pb_type_Speaker_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(pb_type_Speaker,
    MP_QSTR_Speaker,
    MP_TYPE_FLAG_NONE,
    make_new, pb_type_Speaker_make_new,
    locals_dict, &pb_type_Speaker_locals_dict);

#endif // PYBRICKS_PY_COMMON_SPEAKER && MICROPY_PY_BUILTINS_FLOAT
