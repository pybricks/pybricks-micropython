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
    fix16_t counts_per_degree;
    fix16_t counts_per_output_unit;
    uint8_t counter_id;
    pbdrv_counter_dev_t *counter;
} pbio_tacho_t;

pbio_error_t pbio_tacho_get(pbio_port_t port, pbio_tacho_t **tacho, pbio_direction_t direction, fix16_t counts_per_degree, fix16_t gear_ratio);

#endif // _PBIO_TACHO_H_
