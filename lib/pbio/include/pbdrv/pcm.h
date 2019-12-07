// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S


#include <pbio/error.h>

typedef struct _pbdrv_pcm_dev_t pbdrv_pcm_dev_t;

pbio_error_t pbdrv_pcm_get(pbdrv_pcm_dev_t **_pcm_dev);

pbio_error_t pbdrv_pcm_set_volume(pbdrv_pcm_dev_t *pcm_dev, uint32_t volume);

pbio_error_t pbdrv_pcm_play_file_start(pbdrv_pcm_dev_t *pcm_dev, const char *path, int32_t *duration);

pbio_error_t pbdrv_pcm_play_file_update(pbdrv_pcm_dev_t *pcm_dev);

pbio_error_t pbdrv_pcm_play_file_stop(pbdrv_pcm_dev_t *pcm_dev);
