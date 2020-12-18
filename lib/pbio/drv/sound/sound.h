// SPDX-License-Identifier: MIT
// Copyright (c) 2020 The Pybricks Authors

// Sound drivers

#ifndef _INTERNAL_PBDRV_SOUND_H_
#define _INTERNAL_PBDRV_SOUND_H_

#if PBDRV_CONFIG_SOUND

/** Initializes the sound driver. */
void pbdrv_sound_init(void);

#else // PBDRV_CONFIG_SOUND

#define pbdrv_sound_init()

#endif // PBDRV_CONFIG_SOUND

#endif // _INTERNAL_PBDRV_SOUND_H_
