// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <pbio/config.h>

#if PBIO_CONFIG_SOUND

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <contiki.h>

#include <pbdrv/beep.h>
#include <pbdrv/pcm.h>
#include <pbio/error.h>
#include <pbio/sound.h>


struct _pbio_sound_t {
    pbdrv_beep_dev_t *beep_dev;
    pbdrv_pcm_dev_t *pcm_dev;
    bool busy;
    int32_t time_start;
    int32_t time_duration;
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

static pbio_error_t beep_start(pbio_sound_t *sound, uint32_t freq, int32_t duration) {
    // Already started, so return
    if (sound->busy) {
        return PBIO_SUCCESS;
    }

    // Reset state variables
    sound->busy = true;
    sound->time_start = clock_usecs() / 1000;
    sound->time_duration = duration;

    // Start beeping by setting the frequency
    return pbdrv_beep_start_freq(sound->beep_dev, freq);
}

static pbio_error_t beep_stop(pbio_sound_t *sound, pbio_error_t stop_err) {

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

pbio_error_t pbio_sound_beep(pbio_sound_t *sound, uint32_t freq, int32_t duration) {

    pbio_error_t err;

    // If this is called for the first time, init:
    err = beep_start(sound, freq, duration);
    if (err != PBIO_SUCCESS) {
        return beep_stop(sound, err);
    }

    // If we are done, stop
    if (clock_usecs() / 1000 - sound->time_start > duration) {
        return beep_stop(sound, PBIO_SUCCESS);
    }

    // If we are here, we need to call this again
    return PBIO_ERROR_AGAIN;
}

pbio_error_t pbio_sound_set_volume(pbio_sound_t *sound, uint32_t volume) {
    return pbdrv_pcm_set_volume(sound->pcm_dev, volume);
}


static pbio_error_t file_start(pbio_sound_t *sound, const char *path) {

    // Already started, so return
    if (sound->busy) {
        return PBIO_SUCCESS;
    }

    // Reset state variables
    sound->busy = true;
    sound->time_start = clock_usecs() / 1000;

    // Init the sound
    return pbdrv_pcm_play_file_start(sound->pcm_dev, path, &sound->time_duration);
}

static pbio_error_t file_stop(pbio_sound_t *sound, pbio_error_t stop_err) {

    pbio_error_t err;

    // Stop the sound
    err = pbdrv_pcm_play_file_stop(sound->pcm_dev);
    if (err != PBIO_SUCCESS) {
        return err;
    }

    // Return to default state
    sound->busy = false;

    // Return the error that was raised on stopping
    return stop_err;
}

pbio_error_t pbio_sound_play_file(pbio_sound_t *sound, const char *path) {

    pbio_error_t err;
    // If this is called for the first time, init:
    err = file_start(sound, path);
    if (err != PBIO_SUCCESS) {
        return file_stop(sound, err);
    }

    // Update buffer as needed
    err = pbdrv_pcm_play_file_update(sound->pcm_dev);
    if (err == PBIO_ERROR_AGAIN) {
        return err;
    }
    if (err != PBIO_SUCCESS) {
        return file_stop(sound, err);
    }
    // If we are done and the timer is done too, stop
    if (clock_usecs() / 1000 - sound->time_start > sound->time_duration) {
        return file_stop(sound, PBIO_SUCCESS);
    }

    // If we are here, we need to call this again until the sound is done
    return PBIO_ERROR_AGAIN;
}

#endif // PBIO_CONFIG_SOUND
