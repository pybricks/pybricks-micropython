// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#include <pbio/error.h>

#include <pbdrv/pcm.h>

struct _pbdrv_pcm_dev_t {
    int fd;
};

static pbdrv_pcm_dev_t __pcm_dev;

pbio_error_t pbdrv_pcm_get(pbdrv_pcm_dev_t **_pcm_dev) {

    pbdrv_pcm_dev_t *pcm_dev = &__pcm_dev;

    // TODO: Configure & Open

    *_pcm_dev = pcm_dev;

    return PBIO_SUCCESS;
}
