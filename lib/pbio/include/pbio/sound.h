// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdint.h>

#include <pbio/config.h>
#include <pbio/error.h>

typedef struct _pbio_sound_t pbio_sound_t;

#if PBIO_CONFIG_SOUND

pbio_error_t pbio_sound_get(pbio_sound_t **_sound);

pbio_error_t pbio_sound_beep(pbio_sound_t *sound, uint32_t freq, int32_t duration);

pbio_error_t pbio_sound_play_file(pbio_sound_t *sound, const char *path);

pbio_error_t pbio_sound_set_volume(pbio_sound_t *sound, uint32_t volume);

#else

static inline pbio_error_t pbio_sound_get(pbio_sound_t **_sound) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_sound_beep(pbio_sound_t *sound, uint32_t freq, int32_t duration) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_sound_play_file(pbio_sound_t *sound, const char *path) { return PBIO_ERROR_NOT_SUPPORTED; }
static inline pbio_error_t pbio_sound_set_volume(pbio_sound_t *sound, uint32_t volume) { return PBIO_ERROR_NOT_SUPPORTED; }

#endif
