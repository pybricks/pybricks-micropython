// SPDX-License-Identifier: MIT
// Copyright (c) 2025 The Pybricks Authors

// Converts beeps into an array of samples

#include <pbdrv/config.h>

#if PBDRV_CONFIG_SOUND_BEEP_SAMPLED

#include <stddef.h>
#include <stdint.h>

#include <pbdrv/sound.h>
#include <pbio/util.h>

static uint16_t waveform_data[128];

static void pbdrv_sound_generate_square_wave(uint16_t sample_attenuator) {
    uint16_t lo_amplitude_value = INT16_MAX - sample_attenuator;
    uint16_t hi_amplitude_value = sample_attenuator + INT16_MAX;

    size_t i = 0;
    for (; i < PBIO_ARRAY_SIZE(waveform_data) / 2; i++) {
        waveform_data[i] = lo_amplitude_value;
    }
    for (; i < PBIO_ARRAY_SIZE(waveform_data); i++) {
        waveform_data[i] = hi_amplitude_value;
    }
}

// For 0 frequencies that are just flat lines.
static void pbdrv_sound_generate_line_wave(void) {
    for (size_t i = 0; i < PBIO_ARRAY_SIZE(waveform_data); i++) {
        waveform_data[i] = INT16_MAX;
    }
}

void pbdrv_beep_start(uint32_t frequency, uint16_t sample_attenuator) {
    if (frequency == 0) {
        pbdrv_sound_generate_line_wave();
    } else {
        pbdrv_sound_generate_square_wave(sample_attenuator);
    }

    if (frequency < 64) {
        frequency = 64;
    }
    if (frequency > 24000) {
        frequency = 24000;
    }

    pbdrv_sound_start(&waveform_data[0], PBIO_ARRAY_SIZE(waveform_data), frequency * PBIO_ARRAY_SIZE(waveform_data));
}

#endif
