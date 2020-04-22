// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 The Pybricks Authors

#include <stdint.h>

#include <pbio/error.h>

typedef struct _pbdrv_beep_dev_t pbdrv_beep_dev_t;

pbio_error_t pbdrv_beep_get(pbdrv_beep_dev_t **_beep_dev);

pbio_error_t pbdrv_beep_start_freq(pbdrv_beep_dev_t *beep_dev, uint32_t freq);
