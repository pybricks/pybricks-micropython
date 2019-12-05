// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <alsa/asoundlib.h>

#include <pbio/error.h>

#include <pbdrv/pcm.h>

struct _pbdrv_pcm_dev_t {
    int fd;
    snd_mixer_t *snd_mixer;
};

static pbdrv_pcm_dev_t __pcm_dev;

pbio_error_t pbdrv_pcm_get(pbdrv_pcm_dev_t **_pcm_dev) {

    pbdrv_pcm_dev_t *pcm_dev = &__pcm_dev;

    // Open mixer
    if (snd_mixer_open(&pcm_dev->snd_mixer, 0) != 0) {
        return PBIO_ERROR_IO;
    }

    *_pcm_dev = pcm_dev;

    return PBIO_SUCCESS;
}
