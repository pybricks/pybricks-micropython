// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_TACHO_H_
#define _PBIO_TACHO_H_

#include <stdint.h>

#include <fixmath.h>

#include <pbdrv/counter.h>
#include <pbio/dc.h>

typedef struct _pbio_tacho_t {
    pbio_direction_t direction;
    int32_t offset;
    fix16_t counts_per_unit;
    fix16_t counts_per_output_unit;
    pbdrv_counter_dev_t *counter;
} pbio_tacho_t;

#endif // _PBIO_TACHO_H_
