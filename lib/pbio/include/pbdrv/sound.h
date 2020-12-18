// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

/**
 * @addtogroup SoundDriver Driver: Sound
 * @{
 */

#ifndef _PBDRV_SOUND_H_
#define _PBDRV_SOUND_H_

#include <stdint.h>

#include <pbdrv/config.h>
#include <pbio/error.h>


#if PBDRV_CONFIG_SOUND

/**
 * Starts playing a sound repeatedly until pbdrv_sound_stop() is called.
 *
 * @param [in]  data        The PCM data of the sound to play.
 * @param [in]  length      The number of samples in @p data.
 * @param [in]  sample_rate The sample rate of @p data in Hz.
 */
void pbdrv_sound_start(const uint16_t *data, uint32_t length, uint32_t sample_rate);

/**
 * Stops any currently playing sound.
 */
void pbdrv_sound_stop(void);


#else // PBDRV_CONFIG_SOUND

static inline void pbdrv_sound_start(const uint16_t *data, uint32_t length, uint32_t sample_rate) {
}

static inline void pbdrv_sound_stop(void) {
}

#endif // PBDRV_CONFIG_SOUND

#endif // _PBDRV_SOUND_H_

/** @} */
