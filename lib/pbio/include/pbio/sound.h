// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdint.h>

#include <pbio/error.h>

typedef struct _pbio_sound_t pbio_sound_t;

pbio_error_t pbio_sound_get(pbio_sound_t **_sound);

pbio_error_t pbio_sound_beep(pbio_sound_t *sound, uint32_t freq, int32_t duration);

pbio_error_t pbio_sound_play_file(pbio_sound_t *sound, const char *path);

pbio_error_t pbio_sound_set_volume(pbio_sound_t *sound, uint32_t volume);

