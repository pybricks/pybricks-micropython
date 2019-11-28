// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <stdint.h>

#include <pbio/error.h>
#include <pbio/port.h>

typedef struct _pbdrv_sound_dev_t pbdrv_sound_dev_t;

pbio_error_t pbdrv_sound_get(pbdrv_sound_dev_t **_dev);

pbio_error_t pbdrv_sound_beep_freq(pbdrv_sound_dev_t *dev, uint32_t freq);
