// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2019 Laurens Valk
// Copyright (c) 2019 LEGO System A/S

#ifndef _PBIO_DC_H_
#define _PBIO_DC_H_

#include <stdint.h>

typedef enum {
    PBIO_DIRECTION_CLOCKWISE,         /**< Positive means clockwise */
    PBIO_DIRECTION_COUNTERCLOCKWISE,  /**< Positive means counterclockwise */
} pbio_direction_t;

typedef struct _pbio_dc_t {
    pbio_direction_t direction;
    int32_t duty_offset;
    int32_t max_duty_steps;
} pbio_dc_t;

#endif // _PBIO_DC_H_
