// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <pbio/error.h>
#include <pbio/sound.h>

#include <pbdrv/beep.h>
#include <pbdrv/pcm.h>

#include "sys/clock.h"

struct _pbio_sound_t {
    pbdrv_beep_dev_t *beep_dev;
    pbdrv_pcm_dev_t *pcm_dev;
    bool busy;
    int time_start;
};

static pbio_sound_t __sound;

pbio_error_t pbio_sound_get(pbio_sound_t **_sound) {

    pbio_sound_t *sound = &__sound;

    pbio_error_t err;
    
    // Get the beep device
    err = pbdrv_beep_get(&sound->beep_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Stop beeping
    err = pbdrv_beep_start_freq(sound->beep_dev, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Get the PCM device
    err = pbdrv_pcm_get(&sound->pcm_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    *_sound = sound;

    return PBIO_SUCCESS;
}

static pbio_error_t pbio_sound_beep_start(pbio_sound_t *sound, uint32_t freq) {
    // Already started, so return
    if (sound->busy) {
        return PBIO_SUCCESS;
    }

    // Reset state variables
    sound->busy = true;
    sound->time_start = clock_usecs()/1000;

    // Start beeping by setting the frequency
    return pbdrv_beep_start_freq(sound->beep_dev, freq);
}

static pbio_error_t pbio_sound_beep_stop(pbio_sound_t *sound, pbio_error_t stop_err) {

    // Stop the frequency
    pbio_error_t err = pbdrv_beep_start_freq(sound->beep_dev, 0);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return to default state
    sound->busy = false;


    // Return the error that was raised on stopping
    return stop_err;
}

pbio_error_t pbio_sound_beep(pbio_sound_t *sound, uint32_t freq, uint32_t duration) {

    pbio_error_t err;

    // If this is called for the first time, init:
    err = pbio_sound_beep_start(sound, freq);
    if (err != PBIO_SUCCESS) {
        return pbio_sound_beep_stop(sound, err);
    }

    // If we are done, stop
    if (clock_usecs()/1000 - sound->time_start > duration) {
        return pbio_sound_beep_stop(sound, PBIO_SUCCESS);
    }

    // If we are here, we need to call this again
    return PBIO_ERROR_AGAIN;
}

pbio_error_t pbio_sound_set_volume(pbio_sound_t *sound, uint32_t volume) {
    return pbdrv_pcm_set_volume(sound->pcm_dev, volume);
}

pbio_error_t pbio_sound_play_file(pbio_sound_t *sound, const char *path) {
    pbio_error_t err;
    err = pbdrv_pcm_play_file_start(sound->pcm_dev, path);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbdrv_pcm_play_file_update(sound->pcm_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    err = pbdrv_pcm_play_file_stop(sound->pcm_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }
    return PBIO_SUCCESS;
}
