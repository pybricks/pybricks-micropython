// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdint.h>
#include <sndfile.h>
#include <alsa/asoundlib.h>

#include <pbio/error.h>

#include <pbdrv/pcm.h>

struct _pbdrv_pcm_dev_t {
    snd_mixer_t *mixer;
    snd_pcm_t *pcm;
    snd_pcm_uframes_t period_size; // Probably can be kept local, so delete
    snd_pcm_uframes_t buffer_size; // Probably can be kept local, so delete
    snd_mixer_elem_t *beep_elem;
    long beep_vol_min;
    long beep_vol_max;
    snd_mixer_elem_t *pcm_elem;
    long pcm_vol_min;
    long pcm_vol_max;
    SNDFILE *sf;
    SF_INFO sf_info;
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


static pbio_error_t configure_pcm(pbdrv_pcm_dev_t *pcm_dev) {

    // If device has already been opened, return
    if (pcm_dev->pcm != NULL) {
        return PBIO_SUCCESS;
    }

    // Open pcm
    if (snd_pcm_open(
        &pcm_dev->pcm,
        "default",
        SND_PCM_STREAM_PLAYBACK,
        SND_PCM_NONBLOCK) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_pcm_set_volume(pbdrv_pcm_dev_t *pcm_dev, uint32_t volume) {

    if (volume > 100 || volume < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    long pcm_vol = (volume * (pcm_dev->pcm_vol_max - pcm_dev->pcm_vol_min)) / 100 + pcm_dev->pcm_vol_min;
    long beep_vol = (volume * (pcm_dev->beep_vol_max - pcm_dev->beep_vol_min)) / 100 + pcm_dev->beep_vol_min;

    if (snd_mixer_selem_set_playback_volume_all(pcm_dev->pcm_elem, pcm_vol) != 0) {
        return PBIO_ERROR_IO;
    }

    if (snd_mixer_selem_set_playback_volume_all(pcm_dev->beep_elem, beep_vol) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_pcm_play_file_start(pbdrv_pcm_dev_t *pcm_dev, const char *path, int32_t *duration) {

    // Open sound file and get info
    pcm_dev->sf = sf_open(path, SFM_READ, &pcm_dev->sf_info);
    if (pcm_dev->sf == NULL) {
        return PBIO_ERROR_IO;
    }

    // Nominal play duration of the file
    *duration = (pcm_dev->sf_info.frames * 1000) / pcm_dev->sf_info.samplerate;

    // Get hw params
    snd_pcm_hw_params_t *hp;
    if (snd_pcm_hw_params_malloc(&hp) != 0) {
        return PBIO_ERROR_IO;
    }

    // Get the parameters pcm
    if (snd_pcm_hw_params_any(pcm_dev->pcm, hp) != 0) {
        return PBIO_ERROR_IO;
    }

    // Set access
    if (snd_pcm_hw_params_set_access(pcm_dev->pcm, hp, SND_PCM_ACCESS_RW_INTERLEAVED) != 0) {
        return PBIO_ERROR_IO;
    }

    // Set format
    if (snd_pcm_hw_params_set_format(pcm_dev->pcm, hp, SND_PCM_FORMAT_S16_LE) != 0) {
        return PBIO_ERROR_IO;
    }

    // Set channels
    if (snd_pcm_hw_params_set_channels(pcm_dev->pcm, hp, pcm_dev->sf_info.channels) != 0) {
        return PBIO_ERROR_IO;
    }

    // Set rate
    if (snd_pcm_hw_params_set_rate(pcm_dev->pcm, hp, pcm_dev->sf_info.samplerate, 0) != 0) {
        return PBIO_ERROR_IO;
    }

    // Apply params
    if (snd_pcm_hw_params(pcm_dev->pcm, hp) != 0) {
        return PBIO_ERROR_IO;
    }

    // Get period and buffer size
    int dir;
    if (snd_pcm_hw_params_get_period_size(hp, &pcm_dev->period_size, &dir) != 0) {
        return PBIO_ERROR_IO;
    }
    if (snd_pcm_hw_params_get_buffer_size(hp, &pcm_dev->buffer_size) != 0) {
        return PBIO_ERROR_IO;
    }

    // clean up hw params
    snd_pcm_hw_params_free(hp);

    // Prepare the device to make sure it is ready
    if (snd_pcm_prepare(pcm_dev->pcm) != 0) {
        return PBIO_ERROR_IO;
    }

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_pcm_play_file_update(pbdrv_pcm_dev_t *pcm_dev) {

    // Allocate buf
    short *buf = alloca(pcm_dev->period_size * pcm_dev->sf_info.channels * sizeof(short));
    if (buf == NULL) {
        return PBIO_ERROR_FAILED;
    }

    // Check how many frames may write
    sf_count_t avail = snd_pcm_avail_update(pcm_dev->pcm);
    if (avail < 0) {
        return PBIO_ERROR_IO;
    }

    // From the sound file we want to read one period unless less is available to write
    sf_count_t read = avail < pcm_dev->period_size ? avail : pcm_dev->period_size;

    // Read requested number of frames from file
    sf_count_t count = sf_readf_short(pcm_dev->sf, buf, read);
    if (count < 0) {
        return PBIO_ERROR_IO;
    }

    // If there are no frames, we are done
    if (count == 0) {
        return PBIO_SUCCESS;
    }

    // Write the read frames to the sound buffer
    snd_pcm_sframes_t wr = snd_pcm_writei(pcm_dev->pcm, buf, count);
    if (wr <= 0) {
        return PBIO_ERROR_IO;
    }

    // If we read less than a period from the file and write succeeded, we are done
    if (count < read && wr == count) {
        return PBIO_SUCCESS;
    }

    // We need to call this again to read the next chunk of the file and write to sound device
    return PBIO_ERROR_AGAIN;
}

pbio_error_t pbdrv_pcm_play_file_stop(pbdrv_pcm_dev_t *pcm_dev) {
    int err = snd_pcm_drop(pcm_dev->pcm);
    if (errno == EAGAIN) {
        return PBIO_ERROR_AGAIN;
    }
    if (err != 0) {
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

    // Configure pcm
    err = configure_pcm(pcm_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *_pcm_dev = pcm_dev;

    return PBIO_SUCCESS;
}
