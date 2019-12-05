// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2017 David Lechner
// Copyright (c) 2019 LEGO System A/S

#include <stdint.h>

#include <alsa/asoundlib.h>

#include <pbio/error.h>

#include <pbdrv/pcm.h>

struct _pbdrv_pcm_dev_t {
    snd_mixer_t *mixer;
    snd_mixer_elem_t *beep_elem;
    long beep_vol_min;
    long beep_vol_max;
    snd_mixer_elem_t *pcm_elem;
    long pcm_vol_min;
    long pcm_vol_max;
};

static pbdrv_pcm_dev_t __pcm_dev;

static pbio_error_t configure_mixer(pbdrv_pcm_dev_t *pcm_dev) {
    // Open mixer
    if (snd_mixer_open(&pcm_dev->mixer, 0) != 0) {
        return PBIO_ERROR_IO;
    }

    // Use default sound card
    if (snd_mixer_attach(pcm_dev->mixer, "default") != 0) {
        return PBIO_ERROR_IO;
    }
    if (snd_mixer_selem_register(pcm_dev->mixer, 0, 0) != 0) {
        return PBIO_ERROR_IO;
    }
    if (snd_mixer_load(pcm_dev->mixer) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

static pbio_error_t configure_volume_control(pbdrv_pcm_dev_t *pcm_dev) {

    // Get selem ID
    snd_mixer_selem_id_t *selem_id = alloca(snd_mixer_selem_id_sizeof());
    if (selem_id == NULL) {
        return PBIO_ERROR_FAILED;
    }
    memset(selem_id, 0, snd_mixer_selem_id_sizeof());

    // Get PCM volume control
    snd_mixer_selem_id_set_index(selem_id, 0);
    snd_mixer_selem_id_set_name(selem_id, "PCM");
    pcm_dev->pcm_elem = snd_mixer_find_selem(pcm_dev->mixer, selem_id);
    if (pcm_dev->pcm_elem == NULL) {
        return PBIO_ERROR_FAILED;
    }
    if (snd_mixer_selem_get_playback_volume_range(
            pcm_dev->pcm_elem,
            &pcm_dev->pcm_vol_min,
            &pcm_dev->pcm_vol_max) != 0) {
        return PBIO_ERROR_IO;
    }

    // Get beep volume control
    snd_mixer_selem_id_set_index(selem_id, 0);
    snd_mixer_selem_id_set_name(selem_id, "Beep");
    pcm_dev->beep_elem = snd_mixer_find_selem(pcm_dev->mixer, selem_id);
    if (pcm_dev->beep_elem == NULL) {
        return PBIO_ERROR_FAILED;
    }
    if (snd_mixer_selem_get_playback_volume_range(
            pcm_dev->beep_elem,
            &pcm_dev->beep_vol_min,
            &pcm_dev->beep_vol_max) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_pcm_set_volume(pbdrv_pcm_dev_t *pcm_dev, uint32_t volume) {

    if (volume > 100 || volume < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    long pcm_vol = (volume*(pcm_dev->pcm_vol_max - pcm_dev->pcm_vol_min))/100 + pcm_dev->pcm_vol_min;
    long beep_vol = (volume*(pcm_dev->beep_vol_max - pcm_dev->beep_vol_min))/100 + pcm_dev->beep_vol_min;

    if (snd_mixer_selem_set_playback_volume_all(pcm_dev->pcm_elem, pcm_vol) != 0) {
        return PBIO_ERROR_IO;
    }

    if (snd_mixer_selem_set_playback_volume_all(pcm_dev->beep_elem, beep_vol) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}


pbio_error_t pbdrv_pcm_get(pbdrv_pcm_dev_t **_pcm_dev) {

    pbdrv_pcm_dev_t *pcm_dev = &__pcm_dev;
    pbio_error_t err;

    // Configure mixer
    err = configure_mixer(pcm_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get volume control
    err = configure_volume_control(pcm_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *_pcm_dev = pcm_dev;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_pcm_play_file(pbdrv_pcm_dev_t *pcm_dev, const char *path) {
    return PBIO_SUCCESS;
}
